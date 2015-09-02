// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <wangle/channel/broadcast/BroadcastHandler.h>
#include <wangle/channel/broadcast/BroadcastPool.h>
#include <wangle/channel/broadcast/ObservingHandler.h>
#include <wangle/codec/ByteToMessageDecoder.h>

namespace wangle {

class MockBytesToBytesHandler : public wangle::BytesToBytesHandler {
 public:
  MOCK_METHOD1(transportActive, void(Context*));
  MOCK_METHOD1(transportInactive, void(Context*));
  MOCK_METHOD2(read, void(Context*, coral::IOBufQueue&));
  MOCK_METHOD1(readEOF, void(Context*));
  MOCK_METHOD2(readException, void(Context*, coral::exception_wrapper));
  MOCK_METHOD2(write,
               coral::Future<coral::Unit>(Context*,
                                          std::shared_ptr<coral::IOBuf>));
  MOCK_METHOD1(close, coral::Future<coral::Unit>(Context*));

  coral::Future<coral::Unit> write(Context* ctx,
                                   std::unique_ptr<coral::IOBuf> buf) override {
    std::shared_ptr<coral::IOBuf> sbuf(buf.release());
    return write(ctx, sbuf);
  }
};

template <typename T>
class MockSubscriber : public Subscriber<T> {
 public:
  MOCK_METHOD1_T(onNext, void(const T&));
  MOCK_METHOD1(onError, void(const coral::exception_wrapper ex));
  MOCK_METHOD0(onCompleted, void());
};

template <typename T>
class MockByteToMessageDecoder : public ByteToMessageDecoder<T> {
 public:
  typedef typename ByteToMessageDecoder<T>::Context Context;

  MOCK_METHOD4_T(decode, bool(Context*, coral::IOBufQueue&, T&, size_t&));
};

class MockServerPool : public ServerPool {
 public:
  GMOCK_METHOD0_(, noexcept, , getServer, coral::SocketAddress());
};

class MockBroadcastPool : public BroadcastPool<int, std::string> {
 public:
  MockBroadcastPool() : BroadcastPool<int, std::string>(nullptr, nullptr) {}

  MOCK_METHOD1_T(getHandler,
                 coral::Future<BroadcastHandler<int>*>(const std::string&));
};

class MockObservingHandler : public ObservingHandler<int, std::string> {
 public:
  MockObservingHandler()
      : ObservingHandler<int, std::string>("", nullptr, nullptr) {}

  MOCK_METHOD2(write, coral::Future<coral::Unit>(Context*, int));
  MOCK_METHOD1(close, coral::Future<coral::Unit>(Context*));
  MOCK_METHOD0(newBroadcastPool, BroadcastPool<int, std::string>*());
};

class MockBroadcastHandler : public BroadcastHandler<int> {
 public:
  MOCK_METHOD1(subscribe, uint64_t(Subscriber<int>*));
  MOCK_METHOD1(unsubscribe, void(uint64_t));
};

class MockBroadcastPipelineFactory
    : public BroadcastPipelineFactory<int, std::string> {
 public:
  DefaultPipeline::UniquePtr newPipeline(
      std::shared_ptr<coral::AsyncSocket> socket) override {
    DefaultPipeline::UniquePtr pipeline(new DefaultPipeline);
    pipeline->addBack(AsyncSocketHandler(socket));
    pipeline->addBack(std::make_shared<MockByteToMessageDecoder<int>>());
    pipeline->addBack(BroadcastHandler<int>());
    pipeline->finalize();

    return pipeline;
  }

  virtual BroadcastHandler<int>* getBroadcastHandler(
      DefaultPipeline* pipeline) noexcept override {
    return pipeline->getHandler<BroadcastHandler<int>>(2);
  }

  GMOCK_METHOD2_(
      , noexcept, , setRoutingData, void(DefaultPipeline*, const std::string&));
};

} // namespace wangle
