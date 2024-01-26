add_subdirectory(concurrentqueue)
add_subdirectory(pybind11)

set(WITH_GFLAGS OFF)
set(WITH_GTEST OFF)
set(WITH_UNWIND OFF)
set (BUILD_TESTING_SAVED "${BUILD_TESTING}")
set (BUILD_TESTING OFF CACHE BOOL "" FORCE)
set (BUILD_TESTING "${BUILD_TESTING_SAVED}" CACHE BOOL "" FORCE)

add_subdirectory(glog)
# add_subdirectory(gflags)
