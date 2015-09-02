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

#include <wangle/channel/Handler.h>
#include <coral/io/async/AsyncSocket.h>
#include <coral/io/async/EventBase.h>
#include <coral/io/async/EventBaseManager.h>
#include <coral/io/IOBuf.h>
#include <coral/io/IOBufQueue.h>

namespace wangle {

// This handler may only be used in a single Pipeline
class AsyncSocketHandler
  : public wangle::BytesToBytesHandler,
    public coral::AsyncSocket::ReadCallback {
 public:
  explicit AsyncSocketHandler(
      std::shared_ptr<coral::AsyncSocket> socket)
    : socket_(std::move(socket)) {}

  AsyncSocketHandler(AsyncSocketHandler&&) = default;

  ~AsyncSocketHandler() {
    detachReadCallback();
  }

  void attachReadCallback() {
    socket_->setReadCB(socket_->good() ? this : nullptr);
  }

  void detachReadCallback() {
    if (socket_ && socket_->getReadCallback() == this) {
      socket_->setReadCB(nullptr);
    }
    auto ctx = getContext();
    if (ctx && !firedInactive_) {
      firedInactive_ = true;
      ctx->fireTransportInactive();
    }
  }

  void attachEventBase(coral::EventBase* eventBase) {
    if (eventBase && !socket_->getEventBase()) {
      socket_->attachEventBase(eventBase);
    }
  }

  void detachEventBase() {
    detachReadCallback();
    if (socket_->getEventBase()) {
      socket_->detachEventBase();
    }
  }

  void transportActive(Context* ctx) override {
    ctx->getPipeline()->setTransport(socket_);
    attachReadCallback();
    ctx->fireTransportActive();
  }

  void transportInactive(Context* ctx) override {
    detachReadCallback();
    ctx->getPipeline()->setTransport(nullptr);
    ctx->fireTransportInactive();
  }

  void detachPipeline(Context* ctx) override {
    detachReadCallback();
  }

  coral::Future<coral::Unit> write(
      Context* ctx,
      std::unique_ptr<coral::IOBuf> buf) override {
    if (UNLIKELY(!buf)) {
      return coral::makeFuture();
    }

    if (!socket_->good()) {
      VLOG(5) << "socket is closed in write()";
      return coral::makeFuture<coral::Unit>(coral::AsyncSocketException(
          coral::AsyncSocketException::AsyncSocketExceptionType::NOT_OPEN,
          "socket is closed in write()"));
    }

    auto cb = new WriteCallback();
    auto future = cb->promise_.getFuture();
    socket_->writeChain(cb, std::move(buf), ctx->getWriteFlags());
    return future;
  };

  coral::Future<coral::Unit> close(Context* ctx) override {
    if (socket_) {
      detachReadCallback();
      socket_->closeNow();
    }
    ctx->getPipeline()->deletePipeline();
    return coral::makeFuture();
  }

  // Must override to avoid warnings about hidden overloaded virtual due to
  // AsyncSocket::ReadCallback::readEOF()
  void readEOF(Context* ctx) override {
    ctx->fireReadEOF();
  }

  void getReadBuffer(void** bufReturn, size_t* lenReturn) override {
    const auto readBufferSettings = getContext()->getReadBufferSettings();
    const auto ret = bufQueue_.preallocate(
        readBufferSettings.first,
        readBufferSettings.second);
    *bufReturn = ret.first;
    *lenReturn = ret.second;
  }

  void readDataAvailable(size_t len) noexcept override {
    bufQueue_.postallocate(len);
    getContext()->fireRead(bufQueue_);
  }

  void readEOF() noexcept override {
    getContext()->fireReadEOF();
  }

  void readErr(const coral::AsyncSocketException& ex)
    noexcept override {
    getContext()->fireReadException(
        coral::make_exception_wrapper<coral::AsyncSocketException>(ex));
  }

 private:
  class WriteCallback : private coral::AsyncSocket::WriteCallback {
    void writeSuccess() noexcept override {
      promise_.setValue();
      delete this;
    }

    void writeErr(size_t bytesWritten,
                  const coral::AsyncSocketException& ex)
      noexcept override {
      promise_.setException(ex);
      delete this;
    }

   private:
    friend class AsyncSocketHandler;
    coral::Promise<coral::Unit> promise_;
  };

  coral::IOBufQueue bufQueue_{coral::IOBufQueue::cacheChainLength()};
  std::shared_ptr<coral::AsyncSocket> socket_{nullptr};
  bool firedInactive_{false};
};

} // namespace wangle
