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
#include <wangle/service/Service.h>

namespace wangle {

template <typename Pipeline, typename Req, typename Resp = Req>
class ClientDispatcherBase : public HandlerAdapter<Req, Resp>
                             , public Service<Req, Resp> {
 public:
  typedef typename HandlerAdapter<Req, Resp>::Context Context;

  ~ClientDispatcherBase() {
    if (pipeline_) {
      try {
        pipeline_->remove(this).finalize();
      } catch (const std::invalid_argument& e) {
        // not in pipeline; this is fine
      }
    }
  }

  void setPipeline(Pipeline* pipeline) {
    try {
      pipeline->template remove<ClientDispatcherBase>();
    } catch (const std::invalid_argument& e) {
      // no existing dispatcher; this is fine
    }
    pipeline_ = pipeline;
    pipeline_->addBack(this);
    pipeline_->finalize();
  }

  virtual coral::Future<coral::Unit> close() override {
    return HandlerAdapter<Req, Resp>::close(this->getContext());
  }

  virtual coral::Future<coral::Unit> close(Context* ctx) override {
    return HandlerAdapter<Req, Resp>::close(ctx);
  }

 protected:
  Pipeline* pipeline_{nullptr};
};

/**
 * Dispatch a request, satisfying Promise `p` with the response;
 * the returned Future is satisfied when the response is received:
 * only one request is allowed at a time.
 */
template <typename Pipeline, typename Req, typename Resp = Req>
class SerialClientDispatcher
    : public ClientDispatcherBase<Pipeline, Req, Resp> {
 public:
  typedef typename HandlerAdapter<Req, Resp>::Context Context;

  void read(Context* ctx, Req in) override {
    DCHECK(p_);
    p_->setValue(std::move(in));
    p_ = coral::none;
  }

  virtual coral::Future<Resp> operator()(Req arg) override {
    CHECK(!p_);
    DCHECK(this->pipeline_);

    p_ = coral::Promise<Resp>();
    auto f = p_->getFuture();
    this->pipeline_->write(std::move(arg));
    return f;
  }

 private:
  coral::Optional<coral::Promise<Resp>> p_;
};

/**
 * Dispatch a request, satisfying Promise `p` with the response;
 * the returned Future is satisfied when the response is received.
 * A deque of promises/futures are mantained for pipelining.
 */
template <typename Pipeline, typename Req, typename Resp = Req>
class PipelinedClientDispatcher
    : public ClientDispatcherBase<Pipeline, Req, Resp> {
 public:

  typedef typename HandlerAdapter<Req, Resp>::Context Context;

  void read(Context* ctx, Req in) override {
    DCHECK(p_.size() >= 1);
    auto p = std::move(p_.front());
    p_.pop_front();
    p.setValue(std::move(in));
  }

  virtual coral::Future<Resp> operator()(Req arg) override {
    DCHECK(this->pipeline_);

    coral::Promise<Resp> p;
    auto f = p.getFuture();
    p_.push_back(std::move(p));
    this->pipeline_->write(std::move(arg));
    return f;
  }

 private:
  std::deque<coral::Promise<Resp>> p_;
};

/*
 * A full out-of-order request/response client would require some sort
 * of sequence id on the wire.  Currently this is left up to
 * individual protocol writers to implement.
 */

} // namespace wangle