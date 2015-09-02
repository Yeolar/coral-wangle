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

#include <coral/io/async/EventBaseManager.h>
#include <wangle/concurrent/IOExecutor.h>
#include <wangle/concurrent/ThreadPoolExecutor.h>

namespace wangle {

// N.B. For this thread pool, stop() behaves like join() because outstanding
// tasks belong to the event base and will be executed upon its destruction.
class IOThreadPoolExecutor : public ThreadPoolExecutor, public IOExecutor {
 public:
  explicit IOThreadPoolExecutor(
      size_t numThreads,
      std::shared_ptr<ThreadFactory> threadFactory =
          std::make_shared<NamedThreadFactory>("IOThreadPool"),
      coral::EventBaseManager* ebm = coral::EventBaseManager::get());

  ~IOThreadPoolExecutor();

  void add(coral::Func func) override;
  void add(
      coral::Func func,
      std::chrono::milliseconds expiration,
      coral::Func expireCallback = nullptr) override;

  coral::EventBase* getEventBase() override;

  static coral::EventBase* getEventBase(ThreadPoolExecutor::ThreadHandle*);

  coral::EventBaseManager* getEventBaseManager();

 private:
  struct CORAL_ALIGN_TO_AVOID_FALSE_SHARING IOThread : public Thread {
    IOThread(IOThreadPoolExecutor* pool)
      : Thread(pool),
        shouldRun(true),
        pendingTasks(0) {};
    std::atomic<bool> shouldRun;
    std::atomic<size_t> pendingTasks;
    coral::EventBase* eventBase;
  };

  ThreadPtr makeThread() override;
  std::shared_ptr<IOThread> pickThread();
  void threadRun(ThreadPtr thread) override;
  void stopThreads(size_t n) override;
  uint64_t getPendingTaskCount() override;

  size_t nextThread_;
  coral::ThreadLocal<std::shared_ptr<IOThread>> thisThread_;
  coral::EventBaseManager* eventBaseManager_;
};

} // namespace wangle
