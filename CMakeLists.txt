cmake_minimum_required(VERSION 3.17)
project(LightCoroutine)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -stdlib=libc++ -fcoroutines-ts")

include_directories(${PROJECT_SOURCE_DIR})

add_executable(main core/task.h core/future.h main.cc)
