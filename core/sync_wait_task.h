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

#ifndef LIGHTCOROUTINE_SYNC_WAIT_TASK_H_
#define LIGHTCOROUTINE_SYNC_WAIT_TASK_H_

#include <variant>
#include <experimental/coroutine>

#include "core/completion_notifier.h"
#include "core/type_traits.h"

namespace light_coro {

using std::experimental::coroutine_handle;
using std::experimental::suspend_always;

template <typename T>
class SyncWaitTask;

namespace detail {
class SyncWaitTaskPromiseBase {
  friend struct FinalAwaitable;
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }
    template <typename PROMISE>
    void await_suspend(coroutine_handle<PROMISE> coroutine) {
      coroutine.promise().notifier_->Set();
    }
    void await_resume() noexcept {}
  };

 public:
  SyncWaitTaskPromiseBase() noexcept = default;

  auto initial_suspend() noexcept { return suspend_always{}; }

  auto final_suspend() noexcept { return FinalAwaitable{}; }

  // The coroutine should have either yielded a value or thrown
  // an exception in which case it should have bypassed return_void().
  void return_void() noexcept {}

 protected:
  CompletionNotifier* notifier_;
};

template <typename T>
class SyncWaitTaskPromise final : public SyncWaitTaskPromiseBase {
 public:
  SyncWaitTaskPromise() noexcept = default;

  void Start(CompletionNotifier& notifier) {
    notifier_ = &notifier;
    coroutine_handle<SyncWaitTaskPromise<T>>::from_promise(*this).resume();
  }

  SyncWaitTask<T> get_return_object() noexcept;

  auto yield_value(T&& value) {
    result_.template emplace<1>(std::addressof(value));
    return final_suspend();
  }

  void unhandled_exception() noexcept {
    result_.template emplace<2>(std::current_exception());
  }

  T&& result() {
    if (result_.index() == 2)
      std::rethrow_exception(std::get<2>(result_));
    return std::forward<T>(*std::get<1>(result_));
  }

 private:
  std::variant<std::monostate, std::remove_reference_t<T>*, std::exception_ptr> result_;
};

template <>
class SyncWaitTaskPromise<void> : public SyncWaitTaskPromiseBase {
 public:
  SyncWaitTaskPromise() noexcept = default;

  void Start(CompletionNotifier& notifier) {
    notifier_ = &notifier;
    coroutine_handle<SyncWaitTaskPromise<void>>::from_promise(*this).resume();
  }

  SyncWaitTask<void> get_return_object() noexcept;

  void unhandled_exception() noexcept {
    exception_ptr_ = std::current_exception();
  }

  void result() {
    if (exception_ptr_)
      std::rethrow_exception(exception_ptr_);
  }

 private:
  std::exception_ptr exception_ptr_;
};

}  // namespace detail

template <typename T>
class [[nodiscard]] SyncWaitTask final {
 public:
  using promise_type = detail::SyncWaitTaskPromise<T>;

  SyncWaitTask(coroutine_handle<promise_type> coroutine) noexcept
      : coroutine_(coroutine) {}

  SyncWaitTask(SyncWaitTask&& other) noexcept
      : coroutine_(std::exchange(other.coroutine_, coroutine_handle<promise_type>{})) {}

  SyncWaitTask(const SyncWaitTask&) = delete;
  SyncWaitTask& operator=(const SyncWaitTask&) = delete;

  ~SyncWaitTask() {
    if (coroutine_)
      coroutine_.destroy();
  }

  void Start(CompletionNotifier& notifier) noexcept {
    coroutine_.promise().Start(notifier);
  }

  auto Result() {
    return coroutine_.promise().result();
  }

 private:
  coroutine_handle<promise_type> coroutine_;
};

namespace detail {

template <typename T>
inline SyncWaitTask<T> SyncWaitTaskPromise<T>::get_return_object() noexcept {
  return SyncWaitTask<T>{coroutine_handle<SyncWaitTaskPromise>::from_promise(*this)};
}

inline SyncWaitTask<void> SyncWaitTaskPromise<void>::get_return_object() noexcept {
  return SyncWaitTask<void>{coroutine_handle<SyncWaitTaskPromise>::from_promise(*this)};
}

}  // namespace detail

template <
    typename AWAITABLE,
    typename T = typename AwaitableTraits<AWAITABLE&&>::AwaitResultType,
    std::enable_if_t<!std::is_void_v<T>, int> = 0>
SyncWaitTask<T> MakeSyncWaitTask(AWAITABLE&& awaitable) {
  co_yield co_await std::forward<AWAITABLE>(awaitable);
}

template <
    typename AWAITABLE,
    typename T = typename AwaitableTraits<AWAITABLE&&>::AwaitResultType,
    std::enable_if_t<std::is_void_v<T>, int> = 0>
SyncWaitTask<void> MakeSyncWaitTask(AWAITABLE&& awaitable) {
  co_await std::forward<AWAITABLE>(awaitable);
}

template <typename AWAITABLE>
auto SyncWait(AWAITABLE&& awaitable) {
  auto task = MakeSyncWaitTask(std::forward<AWAITABLE>(awaitable));
  CompletionNotifier notifier;
  task.Start(notifier);
  notifier.Wait();
  return task.Result();
}

}  // namespace light_coro

#endif  // LIGHTCOROUTINE_SYNC_WAIT_TASK_H_
