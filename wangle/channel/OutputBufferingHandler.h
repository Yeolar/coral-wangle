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

#include <coral/futures/SharedPromise.h>
#include <wangle/channel/Handler.h>
#include <coral/io/async/EventBase.h>
#include <coral/io/async/EventBaseManager.h>
#include <coral/io/IOBuf.h>
#include <coral/io/IOBufQueue.h>

namespace wangle {

/*
 * OutputBufferingHandler buffers writes in order to minimize syscalls. The
 * transport will be written to once per event loop instead of on every write.
 *
 * This handler may only be used in a single Pipeline.
 */
class OutputBufferingHandler : public OutboundBytesToBytesHandler,
                               protected coral::EventBase::LoopCallback {
 public:
  coral::Future<coral::Unit> write(
      Context* ctx,
      std::unique_ptr<coral::IOBuf> buf) override {
    CHECK(buf);
    if (!queueSends_) {
      return ctx->fireWrite(std::move(buf));
    } else {
      // Delay sends to optimize for fewer syscalls
      if (!sends_) {
        DCHECK(!isLoopCallbackScheduled());
        // Buffer all the sends, and call writev once per event loop.
        sends_ = std::move(buf);
        ctx->getTransport()->getEventBase()->runInLoop(this);
      } else {
        DCHECK(isLoopCallbackScheduled());
        sends_->prependChain(std::move(buf));
      }
      return sharedPromise_.getFuture();
    }
  }

  void runLoopCallback() noexcept override {
    coral::MoveWrapper<coral::SharedPromise<coral::Unit>> sharedPromise;
    std::swap(*sharedPromise, sharedPromise_);
    getContext()->fireWrite(std::move(sends_))
      .then([sharedPromise](coral::Try<coral::Unit> t) mutable {
        sharedPromise->setTry(std::move(t));
      });
  }

  coral::Future<coral::Unit> close(Context* ctx) override {
    if (isLoopCallbackScheduled()) {
      cancelLoopCallback();
    }

    // If there are sends queued, cancel them
    sharedPromise_.setException(
      coral::make_exception_wrapper<std::runtime_error>(
        "close() called while sends still pending"));
    sends_.reset();
    sharedPromise_ = coral::SharedPromise<coral::Unit>();
    return ctx->fireClose();
  }

  coral::SharedPromise<coral::Unit> sharedPromise_;
  std::unique_ptr<coral::IOBuf> sends_{nullptr};
  bool queueSends_{true};
};

} // namespace wangle
