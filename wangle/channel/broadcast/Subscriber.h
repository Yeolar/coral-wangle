// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <coral/ExceptionWrapper.h>

namespace wangle {

/**
 * Subscriber interface for listening to a stream.
 */
template <typename T>
class Subscriber {
 public:
  virtual ~Subscriber() {}

  virtual void onNext(const T&) = 0;
  virtual void onError(coral::exception_wrapper ex) = 0;
  virtual void onCompleted() = 0;
};

} // namespace wangle
