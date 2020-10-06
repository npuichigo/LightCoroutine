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

#ifndef LIGHTCOROUTINE_FUTURE_H_
#define LIGHTCOROUTINE_FUTURE_H_

#include <chrono>
#include <exception>
#include <future>
#include <iostream>
#include <experimental/coroutine>

namespace light_coro {

using std::experimental::coroutine_handle;
using std::experimental::suspend_never;

template <typename T>
struct Future : std::future<T> {
  using std::future<T>::get;
  using std::future<T>::wait;
  using std::future<T>::wait_for;

  Future(std::future<T>&& fut) : std::future<T>(std::move(fut)) {}

  bool await_ready() {
    return wait_for(std::chrono::seconds(0)) ==
        std::future_status::ready;
  }

  void await_suspend(coroutine_handle<> handle) {
    wait();
    handle.resume();
  }

  auto await_resume() {
    return get();
  }

  struct promise_type {
    std::promise<T> promise_;
    Future<T> get_return_object() {
      return promise_.get_future();
    }

    auto initial_suspend() { return suspend_never(); }
    auto final_suspend() { return suspend_never(); }

    template <typename U>
    void return_value(U&& value) {
      promise_.set_value(std::forward<U>(value));
    }

    void unhandled_exception() {
      promise_.set_exception(std::current_exception());
    }
  };
};

template <typename T>
auto async(T&& func) {
  return Future(std::async(std::forward<T>(func)));
}

}  // namespace light_coro

#endif  // LIGHTCOROUTINE_FUTURE_H_
