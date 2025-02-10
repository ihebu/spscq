cmake_minimum_required(VERSION 3.10)

project(spscq)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -Werror -Wno-interference-size -Ofast -march=native")

add_executable(main main.cpp)

target_link_libraries(main PRIVATE pthread)