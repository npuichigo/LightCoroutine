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

#ifndef LIGHTCOROUTINE_TASK_H_
#define LIGHTCOROUTINE_TASK_H_

#include <variant>
#include <experimental/coroutine>

#include "core/completion_notifier.h"
#include "core/sync_wait_task.h"

namespace light_coro {

using std::experimental::coroutine_handle;
using std::experimental::suspend_always;
using std::experimental::suspend_never;

template <typename T>
class Task;

namespace detail {
class TaskPromiseBase {
  friend struct FinalAwaitable;
  struct FinalAwaitable {
    bool await_ready() noexcept { return false; }
    template <typename PROMISE>
    coroutine_handle<> await_suspend(
        coroutine_handle<PROMISE> coroutine) noexcept {
      return coroutine.promise().continuation_;
    }
    void await_resume() noexcept {}
  };

 public:
  TaskPromiseBase() noexcept = default;

  auto initial_suspend() noexcept { return suspend_always{}; }
  auto final_suspend() noexcept { return FinalAwaitable{}; }

  void set_continuation(coroutine_handle<> continuation) noexcept {
    continuation_ = continuation;
  }

 private:
  coroutine_handle<> continuation_;
};

template <typename T>
class TaskPromise final : public TaskPromiseBase {
 public:
  TaskPromise() noexcept = default;

  Task<T> get_return_object() noexcept;

  void unhandled_exception() noexcept {
    result_.template emplace<2>(std::current_exception());
  }

  template <typename U, typename = std::enable_if_t<std::is_convertible_v<U&&, T>>>
  void return_value(U&& value) noexcept(std::is_nothrow_constructible_v<T, U&&>) {
    result_.template emplace<1>(T(std::forward<U>(value)));
  }

  T& result()& {
    if (result_.index() == 2)
      std::rethrow_exception(std::get<2>(result_));
    return std::get<1>(result_);
  }

  T&& result()&& {
    if (result_.index() == 2)
      std::rethrow_exception(std::get<2>(result_));
    return std::move(std::get<1>(result_));
  }

 private:
  std::variant<std::monostate, T, std::exception_ptr> result_;
};

template <>
class TaskPromise<void> : public TaskPromiseBase {
 public:
  TaskPromise() noexcept = default;

  Task<void> get_return_object() noexcept;

  void return_void() noexcept {}

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

template <typename T>
class TaskPromise<T&> : public TaskPromiseBase {
 public:
  TaskPromise() noexcept = default;

  Task<T&> get_return_object() noexcept;

  void unhandled_exception() noexcept {
    result_.emplace<2>(std::current_exception());
  }

  void return_value(T& value) noexcept {
    result_.emplace<1>(std::addressof(value));
  }

  T& result() {
    if (result_.index() == 2)
      std::rethrow_exception(std::get<2>(result_));
    return *std::get<1>(result_);
  }

 private:
  std::variant<std::monostate, T*, std::exception_ptr> result_;
};
}  // namespace detail

template <typename T = void>
class [[nodiscard]] Task {
 public:
  using promise_type = detail::TaskPromise<T>;

 private:
  struct AwaiterBase {
    AwaiterBase(coroutine_handle<promise_type> coroutine)
        : coroutine(coroutine) {}

    bool await_ready() const noexcept {
      return !coroutine || coroutine.done();
    }

    coroutine_handle<> await_suspend(coroutine_handle<> caller_coroutine) {
      coroutine.promise().set_continuation(caller_coroutine);
      return coroutine;
    }

    coroutine_handle<promise_type> coroutine;
  };

 public:
  Task() noexcept = default;

  explicit Task(coroutine_handle<promise_type> coroutine)
      : coroutine_(coroutine) {}

  Task(Task&& other) noexcept
      : coroutine_(std::exchange(other.coroutine_, coroutine_handle<promise_type>{})) {}

  Task(const Task& other) = delete;

  ~Task() {
    if (coroutine_)
      coroutine_.destroy();
  }

  Task& operator=(const Task& other) = delete;
  Task& operator=(Task&& other) noexcept {
    if (std::addressof(other) != this) {
      if (coroutine_)
        coroutine_.destroy();
      coroutine_ = other.coroutine_;
      other.coroutine_ = nullptr;
    }
    return *this;
  }

  auto operator co_await() const& noexcept {
    struct Awaiter : AwaiterBase {
      using AwaiterBase::AwaiterBase;
      auto await_resume() {
        if (!this->coroutine)
          throw std::logic_error("Broken coroutine");
        return this->coroutine.promise().result();
      }
    };
    return Awaiter{coroutine_};
  }

  auto operator co_await() const&& noexcept {
    struct Awaiter : AwaiterBase {
      using AwaiterBase::AwaiterBase;
      auto await_resume() {
        if (!this->coroutine)
          throw std::logic_error("Broken coroutine");
        return std::move(this->coroutine.promise()).result();
      }
    };
    return Awaiter{coroutine_};
  }

  bool IsReady() const noexcept {
    return !coroutine_ || coroutine_.done();
  }

  auto WhenReady() {
    struct Awaiter : AwaiterBase {
      using AwaiterBase::AwaiterBase;
      auto await_resume() {}
    };
    return Awaiter{coroutine_};
  }

 private:
  coroutine_handle<promise_type> coroutine_;
};

namespace detail {
template <typename T>
inline Task<T> TaskPromise<T>::get_return_object() noexcept {
  return Task<T>{coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept {
  return Task<void>{coroutine_handle<TaskPromise>::from_promise(*this)};
}

template <typename T>
inline Task<T&> TaskPromise<T&>::get_return_object() noexcept {
  return Task<T&>{coroutine_handle<TaskPromise>::from_promise(*this)};
}
}  // namespace detail

}  // namespace light_coro

#endif  // LIGHTCOROUTINE_TASK_H_
