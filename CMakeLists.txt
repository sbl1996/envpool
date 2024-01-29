cmake_minimum_required(VERSION 3.18)
project(ygoenv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native")

# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -include ${CMAKE_CURRENT_SOURCE_DIR}/third_party/force_link_glibc_2.17.h")
# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -include ${CMAKE_CURRENT_SOURCE_DIR}/third_party/force_link_glibc_2.17.h")

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

include("${PROJECT_SOURCE_DIR}/cmake/Helper.cmake")

add_subdirectory(third_party)

pybind11_add_module(classic_control_envpool envpool2/classic_control/classic_control.cc)
target_link_libraries(classic_control_envpool PRIVATE glog::glog)
target_include_directories(
    classic_control_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
pybind_move_target(classic_control_envpool envpool2/classic_control)


pybind11_add_module(ygopro_envpool envpool2/ygopro/ygopro.cc)
target_link_libraries(ygopro_envpool PRIVATE glog::glog SQLiteCpp sqlite3 ycore)
target_include_directories(
    ygopro_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
pybind_move_target(ygopro_envpool envpool2/ygopro)
