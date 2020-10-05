/*#include <iostream>

#include "core/sync_wait_task.h"
#include "core/task.h"

using namespace light_coro;

Task<int> Baz() {
  co_return 0;
}

Task<int> Bar() {
  int value = co_await Baz();
  std::cout << "Baz value: " << value << std::endl;
  value++;
  co_return value;
}

Task<int> Foo() {
  int value = co_await Bar();
  std::cout << "Bar value: " << value << std::endl;
  value++;
  co_return value;
}

int main() {
  auto t = Foo();
  t.Resume();
  std::cout << "Foo value: " << t.Result() << std::endl;
  return 0;
}*/

#include <chrono>
#include <iostream>
#include <string>
#include <thread>

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
  std::this_thread::sleep_for(1s);
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
  std::cout << std::endl;
}
