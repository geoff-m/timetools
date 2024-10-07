cmake_minimum_required(VERSION 3.22)
project(timetools CXX ASM)
set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_VERBOSE_MAKEFILE ON CACHE BOOL "ON" FORCE)

add_subdirectory(lib)

add_subdirectory(thirdparty/googletest)
add_subdirectory(test)