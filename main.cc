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

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

#include "core/future.h"
#include "core/task.h"

using std::chrono::high_resolution_clock;
using std::chrono::time_point;
using std::chrono::duration;

using namespace std::chrono_literals;
using namespace light_coro;

auto getTimeSince(const time_point<high_resolution_clock>& start) {
  auto end = high_resolution_clock::now();
  duration<double> elapsed = end - start;
  return elapsed.count();
}

Task<> third(const time_point<high_resolution_clock>& start) {
  auto fut = async([] {
    std::this_thread::sleep_for(1s);
    std::cout << "Async job finished" << std::endl;
  });
  std::this_thread::sleep_for(2s);
  co_await fut;
  std::cout << "Third waited " << getTimeSince(start) << " seconds." << std::endl;
  co_return;
}

Task<> second(const time_point<high_resolution_clock>& start) {
  auto thi = third(start);
  co_await thi;
  std::this_thread::sleep_for(1s);
  std::cout << "Second waited " <<  getTimeSince(start) << " seconds." << std::endl;
}

Task<> first(const time_point<high_resolution_clock>& start) {
  auto sec = second(start);
  co_await sec;
  std::this_thread::sleep_for(1s);
  std::cout << "First waited " <<  getTimeSince(start)  << " seconds." << std::endl;
}

int main() {
  std::cout << std::endl;
  auto start = high_resolution_clock::now();
  auto task = first(start);
  task.Wait();
  std::cout << "Main waited " <<  getTimeSince(start) << " seconds." << std::endl;
}
