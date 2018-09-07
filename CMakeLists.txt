cmake_minimum_required(VERSION 3.1.0)

include_directories(.)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_NAME StormBehavior)
project(${PROJECT_NAME})

enable_testing()
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

add_executable(StormBehaviorTestExe StormBehaviorTest/Main.cpp)
target_link_libraries(StormBehaviorTestExe ${GTEST_LIBRARIES} pthread)

add_test(NAME StormBehaviorTests COMMAND StormBehaviorTestExe)
