cmake_minimum_required(VERSION 3.3)
project(envpool2)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include(${CMAKE_CURRENT_SOURCE_DIR}/PybindHelper.cmake)

add_subdirectory(third_party)

pybind11_add_module(classic_control_envpool envpool2/classic_control/classic_control.cc)
target_link_libraries(classic_control_envpool PRIVATE glog::glog)
target_include_directories(
    classic_control_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/third_party ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party/concurrentqueue)
pybind_move_target(classic_control_envpool envpool2/classic_control)
