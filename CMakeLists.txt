cmake_minimum_required(VERSION 3.18)
project(ygoenv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native -O3")



include("${PROJECT_SOURCE_DIR}/cmake/Helper.cmake")

add_subdirectory(third_party)

pybind11_add_module(ygopro_envpool envpool2/ygopro/ygopro.cpp)
target_link_libraries(ygopro_envpool PRIVATE glog::glog SQLiteCpp sqlite3 ycore unordered_dense::unordered_dense)
target_include_directories(
    ygopro_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
set_target_properties(ygopro_envpool PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
pybind_move_target(ygopro_envpool envpool2/ygopro)


# file(GLOB core_envpool_SRC CONFIGURE_DEPENDS
#      "envpool2/core/*.h"
#      "third_party/SQLiteCpp/include/SQLiteCpp/*.h"
# )
# target_precompile_headers(ygopro_envpool PRIVATE ${core_envpool_SRC})


pybind11_add_module(dummy_envpool envpool2/dummy/dummy_envpool.cpp)
target_link_libraries(dummy_envpool PRIVATE)
target_include_directories(
    dummy_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party)
