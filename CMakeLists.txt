cmake_minimum_required(VERSION 3.18)
project(ygoenv)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native -O3")
set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -march=native -O3")



include("${PROJECT_SOURCE_DIR}/cmake/Helper.cmake")

add_subdirectory(third_party)

pybind11_add_module(ygopro_envpool envpool2/ygopro/ygopro.cpp)
target_link_libraries(ygopro_envpool PRIVATE glog::glog SQLiteCpp sqlite3 ycore unordered_dense::unordered_dense -static-libstdc++)
target_include_directories(
    ygopro_envpool PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

include(CheckIPOSupported)
check_ipo_supported(RESULT result)
if(result)
    set_target_properties(ygopro_envpool PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()

# file(GLOB core_envpool_SRC CONFIGURE_DEPENDS
#      "envpool2/core/*.h"
#      "third_party/SQLiteCpp/include/SQLiteCpp/*.h"
# )
# target_precompile_headers(ygopro_envpool PRIVATE ${core_envpool_SRC})


pybind_move_target(ygopro_envpool envpool2/ygopro)