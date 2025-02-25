cmake_minimum_required(VERSION 3.10)

project(spscq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-interference-size -Ofast -march=native")

add_library(spscq INTERFACE)
target_include_directories(spscq INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE spscq pthread)

include(FetchContent)
FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(
    spscq_test
    tests/spscq_test.cpp
)

target_link_libraries(spscq_test PRIVATE spscq pthread GTest::gtest_main)

include(GoogleTest)
gtest_discover_tests(spscq_test)