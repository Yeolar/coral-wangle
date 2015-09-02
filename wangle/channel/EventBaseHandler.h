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

namespace wangle {

class EventBaseHandler : public OutboundBytesToBytesHandler {
 public:
  coral::Future<coral::Unit> write(
      Context* ctx,
      std::unique_ptr<coral::IOBuf> buf) override {
    coral::Future<coral::Unit> retval;
    DCHECK(ctx->getTransport());
    DCHECK(ctx->getTransport()->getEventBase());
    ctx->getTransport()->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait([&](){
        retval = ctx->fireWrite(std::move(buf));
    });
    return retval;
  }

  coral::Future<coral::Unit> close(Context* ctx) override {
    DCHECK(ctx->getTransport());
    DCHECK(ctx->getTransport()->getEventBase());
    coral::Future<coral::Unit> retval;
    ctx->getTransport()->getEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait([&](){
        retval = ctx->fireClose();
    });
    return retval;
  }
};

} // namespace
