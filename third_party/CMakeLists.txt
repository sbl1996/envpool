add_subdirectory(concurrentqueue)
add_subdirectory(pybind11)
add_subdirectory(unordered_dense)

set(WITH_GFLAGS OFF)
set(WITH_GTEST OFF)
set(WITH_UNWIND OFF)
set (BUILD_TESTING_SAVED "${BUILD_TESTING}")
set (BUILD_TESTING OFF CACHE BOOL "" FORCE)
set (BUILD_TESTING "${BUILD_TESTING_SAVED}" CACHE BOOL "" FORCE)

set(BUILD_SHARED_LIBS OFF)
add_subdirectory(glog)

# set(BUILD_SHARED_LIBS ON)
add_subdirectory(lua-cmake)
set_property(TARGET lua_library PROPERTY POSITION_INDEPENDENT_CODE ON)
# set(BUILD_SHARED_LIBS OFF)

add_subdirectory(SQLiteCpp)


set(YCORE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/ygopro-core)

check_and_insert(${YCORE_DIR}/field.h 14 "#include <cstring>")

file(GLOB ycore_SRC CONFIGURE_DEPENDS
     "${YCORE_DIR}/*.h" "${YCORE_DIR}/*.cpp"
)
add_library(ycore STATIC ${ycore_SRC})
set_property(TARGET ycore PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(ycore PRIVATE lua_library)
