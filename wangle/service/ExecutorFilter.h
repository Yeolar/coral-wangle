#pragma once

#include <wangle/service/Service.h>

namespace wangle {

/**
 * A service that runs all requests through an executor.
 */
template <typename Req, typename Resp = Req>
class ExecutorFilter : public ServiceFilter<Req, Resp> {
 public:
 explicit ExecutorFilter(
   std::shared_ptr<coral::Executor> exe,
   std::shared_ptr<Service<Req, Resp>> service)
      : ServiceFilter<Req, Resp>(service)
      , exe_(exe) {}

 coral::Future<Resp> operator()(Req req) override {
    coral::MoveWrapper<Req> wrapped(std::move(req));
    return via(exe_.get()).then([wrapped,this]() mutable {
      return (*this->service_)(wrapped.move());
    });
  }

 private:
  std::shared_ptr<coral::Executor> exe_;
};

} // namespace wangle
