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

#include <wangle/bootstrap/ServerBootstrap-inl.h>
#include <coral/io/async/AsyncServerSocket.h>
#include <coral/io/async/EventBaseManager.h>
#include <coral/io/async/AsyncUDPServerSocket.h>

namespace wangle {

class ServerSocketFactory {
 public:
  virtual std::shared_ptr<coral::AsyncSocketBase> newSocket(
      int port, coral::SocketAddress address, int backlog,
      bool reuse, ServerSocketConfig& config) = 0;

  virtual void stopSocket(
      std::shared_ptr<coral::AsyncSocketBase>& socket) = 0;

  virtual void removeAcceptCB(
      std::shared_ptr<coral::AsyncSocketBase> sock,
      Acceptor *callback,
      coral::EventBase* base) = 0;

  virtual void addAcceptCB(
      std::shared_ptr<coral::AsyncSocketBase> sock,
      Acceptor* callback,
      coral::EventBase* base) = 0 ;

  virtual ~ServerSocketFactory() = default;
};

class AsyncServerSocketFactory : public ServerSocketFactory {
 public:
  std::shared_ptr<coral::AsyncSocketBase> newSocket(
      int port, coral::SocketAddress address, int /*backlog*/, bool reuse,
      ServerSocketConfig& config) {

    auto socket = coral::AsyncServerSocket::newSocket();
    socket->setReusePortEnabled(reuse);
    socket->attachEventBase(coral::EventBaseManager::get()->getEventBase());
    if (port >= 0) {
      socket->bind(port);
    } else {
      socket->bind(address);
    }

    socket->listen(config.acceptBacklog);
    socket->startAccepting();

    return socket;
  }

  virtual void stopSocket(
    std::shared_ptr<coral::AsyncSocketBase>& s) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncServerSocket>(s);
    DCHECK(socket);
    socket->stopAccepting();
    socket->detachEventBase();
  }

  virtual void removeAcceptCB(std::shared_ptr<coral::AsyncSocketBase> s,
                              Acceptor *callback, coral::EventBase* base) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncServerSocket>(s);
    CHECK(socket);
    socket->removeAcceptCallback(callback, base);
  }

  virtual void addAcceptCB(std::shared_ptr<coral::AsyncSocketBase> s,
                                 Acceptor* callback, coral::EventBase* base) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncServerSocket>(s);
    CHECK(socket);
    socket->addAcceptCallback(callback, base);
  }
};

class AsyncUDPServerSocketFactory : public ServerSocketFactory {
 public:
  std::shared_ptr<coral::AsyncSocketBase> newSocket(
      int port, coral::SocketAddress address, int /*backlog*/, bool reuse,
      ServerSocketConfig& /*config*/) {

    auto socket = std::make_shared<coral::AsyncUDPServerSocket>(
      coral::EventBaseManager::get()->getEventBase());
    socket->setReusePort(reuse);
    if (port >= 0) {
      coral::SocketAddress addressr("::1", port);
      socket->bind(addressr);
    } else {
      socket->bind(address);
    }
    socket->listen();

    return socket;
  }

  virtual void stopSocket(
    std::shared_ptr<coral::AsyncSocketBase>& s) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncUDPServerSocket>(s);
    DCHECK(socket);
    socket->close();
  }

  virtual void removeAcceptCB(std::shared_ptr<coral::AsyncSocketBase> /*s*/,
                              Acceptor* /*callback*/, coral::EventBase* /*base*/) {
  }

  virtual void addAcceptCB(std::shared_ptr<coral::AsyncSocketBase> s,
                                 Acceptor* callback, coral::EventBase* base) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncUDPServerSocket>(s);
    DCHECK(socket);
    socket->addListener(base, callback);
  }
};

} // namespace wangle
