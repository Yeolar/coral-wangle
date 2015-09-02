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

#include <coral/io/async/AsyncTransport.h>
#include <coral/futures/Future.h>
#include <coral/ExceptionWrapper.h>

namespace wangle {

class PipelineBase;

template <class In, class Out>
class HandlerContext {
 public:
  virtual ~HandlerContext() = default;

  virtual void fireRead(In msg) = 0;
  virtual void fireReadEOF() = 0;
  virtual void fireReadException(coral::exception_wrapper e) = 0;
  virtual void fireTransportActive() = 0;
  virtual void fireTransportInactive() = 0;

  virtual coral::Future<coral::Unit> fireWrite(Out msg) = 0;
  virtual coral::Future<coral::Unit> fireClose() = 0;

  virtual PipelineBase* getPipeline() = 0;
  std::shared_ptr<coral::AsyncTransport> getTransport() {
    return getPipeline()->getTransport();
  }

  virtual void setWriteFlags(coral::WriteFlags flags) = 0;
  virtual coral::WriteFlags getWriteFlags() = 0;

  virtual void setReadBufferSettings(
      uint64_t minAvailable,
      uint64_t allocationSize) = 0;
  virtual std::pair<uint64_t, uint64_t> getReadBufferSettings() = 0;

  /* TODO
  template <class H>
  virtual void addHandlerBefore(H&&) {}
  template <class H>
  virtual void addHandlerAfter(H&&) {}
  template <class H>
  virtual void replaceHandler(H&&) {}
  virtual void removeHandler() {}
  */
};

template <class In>
class InboundHandlerContext {
 public:
  virtual ~InboundHandlerContext() = default;

  virtual void fireRead(In msg) = 0;
  virtual void fireReadEOF() = 0;
  virtual void fireReadException(coral::exception_wrapper e) = 0;
  virtual void fireTransportActive() = 0;
  virtual void fireTransportInactive() = 0;

  virtual PipelineBase* getPipeline() = 0;
  std::shared_ptr<coral::AsyncTransport> getTransport() {
    return getPipeline()->getTransport();
  }

  // TODO Need get/set writeFlags, readBufferSettings? Probably not.
  // Do we even really need them stored in the pipeline at all?
  // Could just always delegate to the socket impl
};

template <class Out>
class OutboundHandlerContext {
 public:
  virtual ~OutboundHandlerContext() = default;

  virtual coral::Future<coral::Unit> fireWrite(Out msg) = 0;
  virtual coral::Future<coral::Unit> fireClose() = 0;

  virtual PipelineBase* getPipeline() = 0;
  std::shared_ptr<coral::AsyncTransport> getTransport() {
    return getPipeline()->getTransport();
  }
};

// #include <windows.h> has blessed us with #define IN & OUT, typically mapped
// to nothing, so letting the preprocessor delete each of these symbols, leading
// to interesting compiler errors around HandlerDir.
#ifdef IN
#  undef IN
#endif
#ifdef OUT
#  undef OUT
#endif

enum class HandlerDir {
  IN,
  OUT,
  BOTH
};

} // namespace wangle

#include <wangle/channel/HandlerContext-inl.h>
