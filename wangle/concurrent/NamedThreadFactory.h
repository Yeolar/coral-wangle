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

#include <atomic>
#include <string>
#include <thread>

#include <wangle/concurrent/ThreadFactory.h>
#include <coral/Conv.h>
#include <coral/Range.h>
#include <coral/ThreadName.h>

namespace wangle {

class NamedThreadFactory : public ThreadFactory {
 public:
  explicit NamedThreadFactory(coral::StringPiece prefix)
    : prefix_(prefix.str()), suffix_(0) {}

  std::thread newThread(coral::Func&& func) override {
    auto thread = std::thread(std::move(func));
    coral::setThreadName(
        thread.native_handle(),
        coral::to<std::string>(prefix_, suffix_++));
    return thread;
  }

  void setNamePrefix(coral::StringPiece prefix) {
    prefix_ = prefix.str();
  }

  std::string getNamePrefix() {
    return prefix_;
  }

 private:
  std::string prefix_;
  std::atomic<uint64_t> suffix_;
};

} // namespace wangle
