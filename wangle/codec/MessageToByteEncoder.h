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

namespace wangle {

/**
 * An OutboundHandler which encodes message in a stream-like fashion from one
 * message to IOBuf. Inverse of ByteToMessageDecoder.
 */
template <typename M>
class MessageToByteEncoder : public OutboundHandler<M, std::unique_ptr<coral::IOBuf>> {
 public:
  typedef typename OutboundHandler<M, std::unique_ptr<coral::IOBuf>>::Context Context;

  virtual std::unique_ptr<coral::IOBuf> encode(M& msg) = 0;

  coral::Future<coral::Unit> write(Context* ctx, M msg) override {
    auto buf = encode(msg);
    return buf ? ctx->fireWrite(std::move(buf)) : coral::makeFuture();
  }
};

} // namespace wangle
