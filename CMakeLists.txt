cmake_minimum_required(VERSION 3.10)

project(spscq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-interference-size -Ofast -march=native")

add_library(spscq INTERFACE)
target_include_directories(spscq INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(main src/main.cpp)
target_link_libraries(main PRIVATE spscq pthread)