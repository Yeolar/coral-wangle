/*
 * Copyright 2015 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <coral/Singleton.h>
#include <coral/io/async/AsyncTransport.h>
#include <coral/io/async/AsyncSocket.h>
#include <coral/io/async/NotificationQueue.h>
#include <coral/futures/Future.h>
#include <coral/futures/Promise.h>
#include <wangle/concurrent/IOThreadPoolExecutor.h>

namespace wangle {

class FileRegion {
 public:
  FileRegion(int fd, off_t offset, size_t count)
    : fd_(fd), offset_(offset), count_(count) {}

  coral::Future<coral::Unit> transferTo(
      std::shared_ptr<coral::AsyncTransport> transport) {
    auto socket = std::dynamic_pointer_cast<coral::AsyncSocket>(
        transport);
    CHECK(socket);
    auto cb = new WriteCallback();
    auto f = cb->promise_.getFuture();
    auto req = new FileWriteRequest(socket.get(), cb, fd_, offset_, count_);
    socket->writeRequest(req);
    return f;
  }

 private:
  class WriteCallback : private coral::AsyncSocket::WriteCallback {
    void writeSuccess() noexcept override {
      promise_.setValue();
      delete this;
    }

    void writeErr(size_t bytesWritten,
                  const coral::AsyncSocketException& ex)
      noexcept override {
      promise_.setException(ex);
      delete this;
    }

    friend class FileRegion;
    coral::Promise<coral::Unit> promise_;
  };

  const int fd_;
  const off_t offset_;
  const size_t count_;

  class FileWriteRequest : public coral::AsyncSocket::WriteRequest,
                           public coral::NotificationQueue<size_t>::Consumer {
   public:
    FileWriteRequest(coral::AsyncSocket* socket, WriteCallback* callback,
                     int fd, off_t offset, size_t count);

    void destroy() override;

    bool performWrite() override;

    void consume() override;

    bool isComplete() override;

    void messageAvailable(size_t&& count) override;

    void start() override;

    class FileReadHandler : public coral::EventHandler {
     public:
      FileReadHandler(FileWriteRequest* req, int pipe_in, size_t bytesToRead);

      ~FileReadHandler();

      void handlerReady(uint16_t events) noexcept override;

     private:
      FileWriteRequest* req_;
      int pipe_in_;
      size_t bytesToRead_;
    };

   private:
    ~FileWriteRequest();

    void fail(const char* fn, const coral::AsyncSocketException& ex);

    const int readFd_;
    off_t offset_;
    const size_t count_;
    bool started_{false};
    int pipe_out_{-1};

    size_t bytesInPipe_{0};
    coral::EventBase* readBase_;
    coral::NotificationQueue<size_t> queue_;
    std::unique_ptr<FileReadHandler> readHandler_;
  };
};

} // wangle
