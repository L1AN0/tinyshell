cmake_minimum_required(VERSION 3.6)
project(shell)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
set(CMAKE_BUILD_TYPE Debug)

include_directories(include)
file(GLOB SOURCE_FILES "src/*.cpp")
add_executable(shell ${SOURCE_FILES})
