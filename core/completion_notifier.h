// Copyright 2020 Babble Authors. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// Author: npuichigo@gmail.com (zhangyuchao)

#ifndef LIGHTCOROUTINE_COMPLETION_NOTIFIER_H_
#define LIGHTCOROUTINE_COMPLETION_NOTIFIER_H_

#include <condition_variable>
#include <mutex>
#include <thread>

namespace light_coro {

class CompletionNotifier {
 public:
  CompletionNotifier(bool initial_set = false)
      : is_set_(initial_set) {}

  ~CompletionNotifier() = default;

  void Set() noexcept {
    std::scoped_lock lock(mutex_);
    is_set_ = true;
    cv_.notify_all();
  }

  void Reset() noexcept {
    std::scoped_lock lock(mutex_);
    is_set_ = false;
  }

  void Wait() noexcept {
    std::unique_lock lock(mutex_);
    cv_.wait(lock, [this] { return is_set_; });
  }

 private:
  std::mutex mutex_;
  std::condition_variable cv_;
  bool is_set_;
};

}  // namespace light_coro

#endif  // LIGHTCOROUTINE_COMPLETION_NOTIFIER_H_
