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

#ifndef LIGHTCOROUTINE_TYPE_TRAITS_H_
#define LIGHTCOROUTINE_TYPE_TRAITS_H_

#include <any>
#include <experimental/coroutine>

namespace light_coro {
namespace detail {

using std::experimental::coroutine_handle;

template <typename T>
struct IsCoroutineHandle : std::false_type {};

template <typename PROMISE>
struct IsCoroutineHandle<coroutine_handle<PROMISE>> : std::true_type {};

template <typename T>
struct IsValidAwaitSuspendReturnValue
    : std::disjunction<std::is_void<T>, std::is_same<T, bool>, IsCoroutineHandle<T>> {};

template <typename T, typename = std::void_t<>>
struct IsAwaiter : std::false_type {};

template <typename T>
struct IsAwaiter<
    T,
    std::void_t<
        decltype(std::declval<T>().await_ready()),
        decltype(std::declval<T>().await_suspend(
            std::declval<std::experimental::coroutine_handle<>>())),
        decltype(std::declval<T>().await_resume())>>
    : std::conjunction<
        std::is_constructible<bool, decltype(std::declval<T>().await_ready())>,
        IsValidAwaitSuspendReturnValue<decltype(
            std::declval<T>().await_suspend(
                std::declval<coroutine_handle<>>()))>> {};

template <typename T>
auto GetAwaiterImpl(T&& value, int) noexcept(
    noexcept(static_cast<T&&>(value).operator co_await())) {
  return static_cast<T&&>(value).operator co_await();
}

template <typename T>
auto GetAwaiterImpl(T&& value, long) noexcept(
    noexcept(operator co_await(static_cast<T&&>(value)))) {
  return operator co_await(static_cast<T&&>(value));
}

template <typename T, std::enable_if_t<IsAwaiter<T&&>::value, int> = 0>
T&& GetAwaiterImpl(T&& value, std::any) noexcept {
  return static_cast<T&&>(value);
}

template <typename T>
auto GetAwaiter(T&& value) noexcept(
    noexcept(GetAwaiterImpl(static_cast<T&&>(value), int()))) {
  return GetAwaiterImpl(static_cast<T&&>(value), int());
}

}  // namespace detail

template <typename T, typename = void>
struct AwaitableTraits {
};

template <typename T>
struct AwaitableTraits<
    T,
    std::void_t<decltype(detail::GetAwaiter(std::declval<T>()))>> {
  using AwaitType = decltype(detail::GetAwaiter(std::declval<T>()));
  using AwaitResultType = decltype(std::declval<AwaitType>().await_resume());
};

}  // namespace light_coro

#endif  // LIGHTCOROUTINE_TYPE_TRAITS_H_
