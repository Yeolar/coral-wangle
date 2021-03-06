// Copyright 2004-present Facebook.  All rights reserved.
#include <wangle/channel/broadcast/test/Mocks.h>

using namespace wangle;
using namespace folly;
using namespace testing;

class BroadcastHandlerTest : public Test {
 public:
  class MockBroadcastHandler : public BroadcastHandler<std::string> {
   public:
    MOCK_METHOD1(close, folly::Future<folly::Unit>(Context*));
  };

  void SetUp() override {
    prevHandler = new StrictMock<MockBytesToBytesHandler>();
    EXPECT_CALL(*prevHandler, read(_, _))
        .WillRepeatedly(Invoke([&](MockBytesToBytesHandler::Context* ctx,
                                   IOBufQueue& q) { ctx->fireRead(q); }));

    decoder = new StrictMock<MockByteToMessageDecoder<std::string>>();
    handler = new StrictMock<MockBroadcastHandler>();

    pipeline.reset(new DefaultPipeline);
    pipeline->addBack(
        std::shared_ptr<StrictMock<MockBytesToBytesHandler>>(prevHandler));
    pipeline->addBack(
        std::shared_ptr<StrictMock<MockByteToMessageDecoder<std::string>>>(
            decoder));
    pipeline->addBack(
        std::shared_ptr<StrictMock<MockBroadcastHandler>>(handler));
    pipeline->finalize();
  }

  void TearDown() override {
    Mock::VerifyAndClear(&subscriber0);
    Mock::VerifyAndClear(&subscriber1);

    pipeline.reset();
  }

 protected:
  DefaultPipeline::UniquePtr pipeline;

  StrictMock<MockBytesToBytesHandler>* prevHandler{nullptr};
  StrictMock<MockByteToMessageDecoder<std::string>>* decoder{nullptr};
  StrictMock<MockBroadcastHandler>* handler{nullptr};

  StrictMock<MockSubscriber<std::string>> subscriber0;
  StrictMock<MockSubscriber<std::string>> subscriber1;
};

TEST_F(BroadcastHandlerTest, SubscribeUnsubscribe) {
  // Test by adding a couple of subscribers and unsubscribing them
  EXPECT_CALL(*decoder, decode(_, _, _, _))
      .WillRepeatedly(
          Invoke([&](MockByteToMessageDecoder<std::string>::Context*,
                     IOBufQueue& q, std::string& data, size_t&) {
            auto buf = q.move();
            if (buf) {
              buf->coalesce();
              data = buf->moveToFbString().toStdString();
              return true;
            }
            return false;
          }));

  InSequence dummy;

  // Add a subscriber
  EXPECT_EQ(handler->subscribe(&subscriber0), 0);

  EXPECT_CALL(subscriber0, onNext("data1")).Times(1);
  EXPECT_CALL(subscriber0, onNext("data2")).Times(1);

  // Push some data
  IOBufQueue q;
  q.append(IOBuf::copyBuffer("data1"));
  pipeline->read(q);
  q.clear();
  q.append(IOBuf::copyBuffer("data2"));
  pipeline->read(q);
  q.clear();

  // Add another subscriber
  EXPECT_EQ(handler->subscribe(&subscriber1), 1);

  EXPECT_CALL(subscriber0, onNext("data3")).Times(1);
  EXPECT_CALL(subscriber1, onNext("data3")).Times(1);

  // Push more data
  q.append(IOBuf::copyBuffer("data3"));
  pipeline->read(q);
  q.clear();

  // Unsubscribe one of the subscribers
  handler->unsubscribe(0);

  EXPECT_CALL(subscriber1, onNext(Eq("data4"))).Times(1);

  // Push more data
  q.append(IOBuf::copyBuffer("data4"));
  pipeline->read(q);
  q.clear();

  EXPECT_CALL(*handler, close(_))
      .WillOnce(InvokeWithoutArgs([this] {
        pipeline.reset();
        return makeFuture();
      }));

  // Unsubscribe the other subscriber. The handler should be deleted now.
  handler->unsubscribe(1);
}

TEST_F(BroadcastHandlerTest, BufferedRead) {
  // Test with decoder that buffers data based on some local logic
  // before pushing to subscribers
  IOBufQueue bufQueue{IOBufQueue::cacheChainLength()};
  EXPECT_CALL(*decoder, decode(_, _, _, _))
      .WillRepeatedly(
          Invoke([&](MockByteToMessageDecoder<std::string>::Context*,
                     IOBufQueue& q, std::string& data, size_t&) {
            bufQueue.append(q);
            if (bufQueue.chainLength() < 5) {
              return false;
            }
            auto buf = bufQueue.move();
            buf->coalesce();
            data = buf->moveToFbString().toStdString();
            return true;
          }));

  InSequence dummy;

  // Add a subscriber
  EXPECT_EQ(handler->subscribe(&subscriber0), 0);

  EXPECT_CALL(subscriber0, onNext("data1")).Times(1);

  // Push some fragmented data
  IOBufQueue q;
  q.append(IOBuf::copyBuffer("da"));
  pipeline->read(q);
  q.clear();
  q.append(IOBuf::copyBuffer("ta1"));
  pipeline->read(q);
  q.clear();

  // Push more fragmented data. onNext shouldn't be called yet.
  q.append(IOBuf::copyBuffer("dat"));
  pipeline->read(q);
  q.clear();
  q.append(IOBuf::copyBuffer("a"));
  pipeline->read(q);
  q.clear();

  // Add another subscriber
  EXPECT_EQ(handler->subscribe(&subscriber1), 1);

  EXPECT_CALL(subscriber0, onNext("data3data4")).Times(1);
  EXPECT_CALL(subscriber1, onNext("data3data4")).Times(1);

  // Push rest of the fragmented data. The entire data should be pushed
  // to both subscribers.
  q.append(IOBuf::copyBuffer("3data4"));
  pipeline->read(q);
  q.clear();

  EXPECT_CALL(subscriber0, onNext("data2")).Times(1);
  EXPECT_CALL(subscriber1, onNext("data2")).Times(1);

  // Push some unfragmented data
  q.append(IOBuf::copyBuffer("data2"));
  pipeline->read(q);
  q.clear();

  EXPECT_CALL(*handler, close(_))
      .WillOnce(InvokeWithoutArgs([this] {
        pipeline.reset();
        return makeFuture();
      }));

  // Unsubscribe all subscribers. The handler should be deleted now.
  handler->unsubscribe(0);
  handler->unsubscribe(1);
}

TEST_F(BroadcastHandlerTest, OnCompleted) {
  // Test with EOF on the handler
  EXPECT_CALL(*decoder, decode(_, _, _, _))
      .WillRepeatedly(
          Invoke([&](MockByteToMessageDecoder<std::string>::Context*,
                     IOBufQueue& q, std::string& data, size_t&) {
            auto buf = q.move();
            if (buf) {
              buf->coalesce();
              data = buf->moveToFbString().toStdString();
              return true;
            }
            return false;
          }));

  InSequence dummy;

  // Add a subscriber
  EXPECT_EQ(handler->subscribe(&subscriber0), 0);

  EXPECT_CALL(subscriber0, onNext("data1")).Times(1);

  // Push some data
  IOBufQueue q;
  q.append(IOBuf::copyBuffer("data1"));
  pipeline->read(q);
  q.clear();

  // Add another subscriber
  EXPECT_EQ(handler->subscribe(&subscriber1), 1);

  EXPECT_CALL(subscriber0, onNext("data2")).Times(1);
  EXPECT_CALL(subscriber1, onNext("data2")).Times(1);

  // Push more data
  q.append(IOBuf::copyBuffer("data2"));
  pipeline->read(q);
  q.clear();

  // Unsubscribe one of the subscribers
  handler->unsubscribe(0);

  EXPECT_CALL(subscriber1, onCompleted()).Times(1);

  EXPECT_CALL(*handler, close(_))
      .WillOnce(InvokeWithoutArgs([this] {
        pipeline.reset();
        return makeFuture();
      }));

  // The handler should be deleted now
  handler->readEOF(nullptr);
}

TEST_F(BroadcastHandlerTest, OnError) {
  // Test with EOF on the handler
  EXPECT_CALL(*decoder, decode(_, _, _, _))
      .WillRepeatedly(
          Invoke([&](MockByteToMessageDecoder<std::string>::Context*,
                     IOBufQueue& q, std::string& data, size_t&) {
            auto buf = q.move();
            if (buf) {
              buf->coalesce();
              data = buf->moveToFbString().toStdString();
              return true;
            }
            return false;
          }));

  InSequence dummy;

  // Add a subscriber
  EXPECT_EQ(handler->subscribe(&subscriber0), 0);

  EXPECT_CALL(subscriber0, onNext("data1")).Times(1);

  // Push some data
  IOBufQueue q;
  q.append(IOBuf::copyBuffer("data1"));
  pipeline->read(q);
  q.clear();

  // Add another subscriber
  EXPECT_EQ(handler->subscribe(&subscriber1), 1);

  EXPECT_CALL(subscriber0, onNext("data2")).Times(1);
  EXPECT_CALL(subscriber1, onNext("data2")).Times(1);

  // Push more data
  q.append(IOBuf::copyBuffer("data2"));
  pipeline->read(q);
  q.clear();

  EXPECT_CALL(subscriber0, onError(_)).Times(1);
  EXPECT_CALL(subscriber1, onError(_)).Times(1);

  EXPECT_CALL(*handler, close(_))
      .WillOnce(InvokeWithoutArgs([this] {
        pipeline.reset();
        return makeFuture();
      }));

  // The handler should be deleted now
  handler->readException(nullptr, make_exception_wrapper<std::exception>());
}
