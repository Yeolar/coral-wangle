#pragma once

#include <chrono>
#include <coral/SocketAddress.h>
#include <coral/io/async/AsyncSocket.h>
#include <wangle/acceptor/Acceptor.h>
#include <wangle/acceptor/ManagedConnection.h>
#include <wangle/acceptor/TransportInfo.h>

namespace wangle {

class AcceptorHandshakeHelper : public coral::AsyncSSLSocket::HandshakeCB,
                                public ManagedConnection {
  public:
    AcceptorHandshakeHelper(
        coral::AsyncSSLSocket::UniquePtr sock,
        Acceptor* acceptor,
        const coral::SocketAddress& clientAddr,
        std::chrono::steady_clock::time_point acceptTime,
        TransportInfo tinfo) :
      socket_(std::move(sock)), acceptor_(acceptor),
      clientAddr_(clientAddr), acceptTime_(acceptTime),
      tinfo_(std::move(tinfo)) {
      acceptor_->downstreamConnectionManager_->addConnection(this, true);
      if (acceptor_->parseClientHello_) {
        socket_->enableClientHelloParsing();
      }
    }

    virtual void start() noexcept;

    void timeoutExpired() noexcept override {
      VLOG(4) << "SSL handshake timeout expired";
      sslError_ = SSLErrorEnum::TIMEOUT;
      dropConnection();
    }

    void describe(std::ostream& os) const override {
      os << "pending handshake on " << clientAddr_;
    }

    bool isBusy() const override { return true; }

    void notifyPendingShutdown() override {}

    void closeWhenIdle() override {}

    void dropConnection() override {
      VLOG(10) << "Dropping in progress handshake for " << clientAddr_;
      socket_->closeNow();
    }

    void dumpConnectionState(uint8_t loglevel) override {}

  protected:
    // AsyncSSLSocket::HandshakeCallback API
    void handshakeSuc(coral::AsyncSSLSocket* sock) noexcept override;
    void handshakeErr(coral::AsyncSSLSocket* sock,
                      const coral::AsyncSocketException& ex) noexcept override;

    coral::AsyncSSLSocket::UniquePtr socket_;
    Acceptor* acceptor_;
    coral::SocketAddress clientAddr_;
    std::chrono::steady_clock::time_point acceptTime_;
    TransportInfo tinfo_;
    SSLErrorEnum sslError_{SSLErrorEnum::NO_ERROR};
};

}
