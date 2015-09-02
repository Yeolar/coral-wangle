/*
 *  Copyright (c) 2015, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <wangle/acceptor/ServerSocketConfig.h>
#include <wangle/acceptor/ConnectionCounter.h>
#include <wangle/acceptor/ConnectionManager.h>
#include <wangle/acceptor/LoadShedConfiguration.h>
#include <wangle/acceptor/SecureTransportType.h>
#include <wangle/ssl/SSLCacheProvider.h>
#include <wangle/acceptor/TransportInfo.h>

#include <chrono>
#include <event.h>
#include <coral/io/async/AsyncSSLSocket.h>
#include <coral/io/async/AsyncServerSocket.h>
#include <coral/io/async/AsyncUDPServerSocket.h>

namespace wangle {

class AsyncTransport;
class ManagedConnection;
class SSLContextManager;

/**
 * An abstract acceptor for TCP-based network services.
 *
 * There is one acceptor object per thread for each listening socket.  When a
 * new connection arrives on the listening socket, it is accepted by one of the
 * acceptor objects.  From that point on the connection will be processed by
 * that acceptor's thread.
 *
 * The acceptor will call the abstract onNewConnection() method to create
 * a new ManagedConnection object for each accepted socket.  The acceptor
 * also tracks all outstanding connections that it has accepted.
 */
class Acceptor :
  public coral::AsyncServerSocket::AcceptCallback,
  public wangle::ConnectionManager::Callback,
  public coral::AsyncUDPServerSocket::Callback  {
 public:

  enum class State : uint32_t {
    kInit,  // not yet started
    kRunning, // processing requests normally
    kDraining, // processing outstanding conns, but not accepting new ones
    kDone,  // no longer accepting, and all connections finished
  };

  explicit Acceptor(const ServerSocketConfig& accConfig);
  virtual ~Acceptor();

  /**
   * Supply an SSL cache provider
   * @note Call this before init()
   */
  virtual void setSSLCacheProvider(
      const std::shared_ptr<SSLCacheProvider>& cacheProvider) {
    cacheProvider_ = cacheProvider;
  }

  /**
   * Initialize the Acceptor to run in the specified EventBase
   * thread, receiving connections from the specified AsyncServerSocket.
   *
   * This method will be called from the AsyncServerSocket's primary thread,
   * not the specified EventBase thread.
   */
  virtual void init(coral::AsyncServerSocket* serverSocket,
                    coral::EventBase* eventBase);

  /**
   * Dynamically add a new SSLContextConfig
   */
  void addSSLContextConfig(const SSLContextConfig& sslCtxConfig);

  SSLContextManager* getSSLContextManager() const {
    return sslCtxManager_.get();
  }

  /**
   * Return the number of outstanding connections in this service instance.
   */
  uint32_t getNumConnections() const {
    return downstreamConnectionManager_ ?
      (uint32_t)downstreamConnectionManager_->getNumConnections() : 0;
  }

  /**
   * Access the Acceptor's event base.
   */
  virtual coral::EventBase* getEventBase() const { return base_; }

  /**
   * Access the Acceptor's downstream (client-side) ConnectionManager
   */
  virtual wangle::ConnectionManager* getConnectionManager() {
    return downstreamConnectionManager_.get();
  }

  /**
   * Invoked when a new ManagedConnection is created.
   *
   * This allows the Acceptor to track the outstanding connections,
   * for tracking timeouts and for ensuring that all connections have been
   * drained on shutdown.
   */
  void addConnection(wangle::ManagedConnection* connection);

  /**
   * Get this acceptor's current state.
   */
  State getState() const {
    return state_;
  }

  /**
   * Get the current connection timeout.
   */
  std::chrono::milliseconds getConnTimeout() const;

  /**
   * Returns the name of this VIP.
   *
   * Will return an empty string if no name has been configured.
   */
  const std::string& getName() const {
    return accConfig_.name;
  }

  /**
   * Force the acceptor to drop all connections and stop processing.
   *
   * This function may be called from any thread.  The acceptor will not
   * necessarily stop before this function returns: the stop will be scheduled
   * to run in the acceptor's thread.
   */
  virtual void forceStop();

  bool isSSL() const { return accConfig_.isSSL(); }

  const ServerSocketConfig& getConfig() const { return accConfig_; }

  static uint64_t getTotalNumPendingSSLConns() {
    return totalNumPendingSSLConns_.load();
  }

  /**
   * Called right when the TCP connection has been accepted, before processing
   * the first HTTP bytes (HTTP) or the SSL handshake (HTTPS)
   */
  virtual void onDoneAcceptingConnection(
    int fd,
    const coral::SocketAddress& clientAddr,
    std::chrono::steady_clock::time_point acceptTime
  ) noexcept;

  /**
   * Begins either processing HTTP bytes (HTTP) or the SSL handshake (HTTPS)
   */
  void processEstablishedConnection(
    int fd,
    const coral::SocketAddress& clientAddr,
    std::chrono::steady_clock::time_point acceptTime,
    TransportInfo& tinfo
  ) noexcept;

  /**
   * Creates and starts the handshake helper.
   */
  virtual void startHandshakeHelper(
    coral::AsyncSSLSocket::UniquePtr sslSock,
    Acceptor* acceptor,
    const coral::SocketAddress& clientAddr,
    std::chrono::steady_clock::time_point acceptTime,
    TransportInfo& tinfo) noexcept;

  /**
   * Drains all open connections of their outstanding transactions. When
   * a connection's transaction count reaches zero, the connection closes.
   */
  void drainAllConnections();

  /**
   * Drop all connections.
   *
   * forceStop() schedules dropAllConnections() to be called in the acceptor's
   * thread.
   */
  void dropAllConnections();

  /**
   * Wrapper for connectionReady() that decrements the count of
   * pending SSL connections. This should normally not be overridden.
   */
  virtual void sslConnectionReady(coral::AsyncSocket::UniquePtr sock,
      const coral::SocketAddress& clientAddr,
      const std::string& nextProtocol,
      SecureTransportType secureTransportType,
      TransportInfo& tinfo);

  /**
   * Notification callback for SSL handshake failures.
   */
  virtual void sslConnectionError();

 protected:
  friend class AcceptorHandshakeHelper;

  /**
   * Our event loop.
   *
   * Probably needs to be used to pass to a ManagedConnection
   * implementation. Also visible in case a subclass wishes to do additional
   * things w/ the event loop (e.g. in attach()).
   */
  coral::EventBase* base_{nullptr};

  virtual uint64_t getConnectionCountForLoadShedding(void) const { return 0; }

  /**
   * Hook for subclasses to drop newly accepted connections prior
   * to handshaking.
   */
  virtual bool canAccept(const coral::SocketAddress&);

  /**
   * Invoked when a new connection is created. This is where application starts
   * processing a new downstream connection.
   *
   * NOTE: Application should add the new connection to
   *       downstreamConnectionManager so that it can be garbage collected after
   *       certain period of idleness.
   *
   * @param sock                the socket connected to the client
   * @param address             the address of the client
   * @param nextProtocolName    the name of the L6 or L7 protocol to be
   *                              spoken on the connection, if known (e.g.,
   *                              from TLS NPN during secure connection setup),
   *                              or an empty string if unknown
   * @param secureTransportType the name of the secure transport type that was
   *                            requested by the client.
   */
  virtual void onNewConnection(
      coral::AsyncSocket::UniquePtr /*sock*/,
      const coral::SocketAddress* /*address*/,
      const std::string& /*nextProtocolName*/,
      SecureTransportType /*secureTransportType*/,
      const TransportInfo& /*tinfo*/) {}

  void onListenStarted() noexcept {}
  void onListenStopped() noexcept {}
  void onDataAvailable(
    std::shared_ptr<coral::AsyncUDPSocket> /*socket*/,
    const coral::SocketAddress&,
    std::unique_ptr<coral::IOBuf>, bool) noexcept {}

  virtual coral::AsyncSocket::UniquePtr makeNewAsyncSocket(
      coral::EventBase* base,
      int fd) {
    return coral::AsyncSocket::UniquePtr(
        new coral::AsyncSocket(base, fd));
  }

  virtual coral::AsyncSSLSocket::UniquePtr makeNewAsyncSSLSocket(
    const std::shared_ptr<coral::SSLContext>& ctx, coral::EventBase* base, int fd) {
    return coral::AsyncSSLSocket::UniquePtr(
        new coral::AsyncSSLSocket(
          ctx,
          base,
          fd,
          true, /* set server */
          true /* defer the security negotiation until sslAccept */));
  }

  /**
   * Hook for subclasses to record stats about SSL connection establishment.
   */
  virtual void updateSSLStats(
      const coral::AsyncSSLSocket* /*sock*/,
      std::chrono::milliseconds /*acceptLatency*/,
      SSLErrorEnum /*error*/) noexcept {}

 protected:

  /**
   * onConnectionsDrained() will be called once all connections have been
   * drained while the acceptor is stopping.
   *
   * Subclasses can override this method to perform any subclass-specific
   * cleanup.
   */
  virtual void onConnectionsDrained() {}

  // AsyncServerSocket::AcceptCallback methods
  void connectionAccepted(int fd,
      const coral::SocketAddress& clientAddr)
      noexcept;
  void acceptError(const std::exception& ex) noexcept;
  void acceptStopped() noexcept;

  // ConnectionManager::Callback methods
  void onEmpty(const wangle::ConnectionManager& cm);
  void onConnectionAdded(const wangle::ConnectionManager& /*cm*/) {}
  void onConnectionRemoved(const wangle::ConnectionManager& /*cm*/) {}

  /**
   * Process a connection that is to ready to receive L7 traffic.
   * This method is called immediately upon accept for plaintext
   * connections and upon completion of SSL handshaking or resumption
   * for SSL connections.
   */
   void connectionReady(
      coral::AsyncSocket::UniquePtr sock,
      const coral::SocketAddress& clientAddr,
      const std::string& nextProtocolName,
      SecureTransportType secureTransportType,
      TransportInfo& tinfo);

  const LoadShedConfiguration& getLoadShedConfiguration() const {
    return loadShedConfig_;
  }

 protected:
  const ServerSocketConfig accConfig_;
  void setLoadShedConfig(const LoadShedConfiguration& from,
                         IConnectionCounter* counter);

  /**
   * Socket options to apply to the client socket
   */
  coral::AsyncSocket::OptionMap socketOptions_;

  std::unique_ptr<SSLContextManager> sslCtxManager_;

  /**
   * Whether we want to enable client hello parsing in the handshake helper
   * to get list of supported client ciphers.
   */
  bool parseClientHello_{false};

  wangle::ConnectionManager::UniquePtr downstreamConnectionManager_;

 private:

  // Forbidden copy constructor and assignment opererator
  Acceptor(Acceptor const &) = delete;
  Acceptor& operator=(Acceptor const &) = delete;

  void checkDrained();

  State state_{State::kInit};
  uint64_t numPendingSSLConns_{0};

  static std::atomic<uint64_t> totalNumPendingSSLConns_;

  bool forceShutdownInProgress_{false};
  LoadShedConfiguration loadShedConfig_;
  IConnectionCounter* connectionCounter_{nullptr};
  std::shared_ptr<SSLCacheProvider> cacheProvider_;
};

class AcceptorFactory {
 public:
  virtual std::shared_ptr<Acceptor> newAcceptor(coral::EventBase*) = 0;
  virtual ~AcceptorFactory() = default;
};

} // namespace
