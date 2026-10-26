cmake_minimum_required(VERSION 3.20)
project(MetiC++ VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Base optimization flags (No object-oriented bloat, maximum inline)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread -Wall -Wextra")

# Core source files
file(GLOB_RECURSE SRC_FILES "src/*.cpp")

# Target 1: Standard Modern CPU (AVX2 / x86-64-v3)
add_executable(meticpp ${SRC_FILES})
target_compile_options(meticpp PRIVATE $<$<CONFIG:Release>:-O3;-flto;-DNDEBUG>)
option(METI_NATIVE "Enable host-specific CPU instructions" OFF)
if(METI_NATIVE)
	target_compile_options(meticpp PRIVATE -march=native)
endif()

enable_testing()
add_executable(meticpp_tests tests/perft.cpp src/move_logic.cpp)
target_include_directories(meticpp_tests PRIVATE src)
target_compile_options(meticpp_tests PRIVATE -Wall -Wextra)
add_test(NAME meticpp_perft COMMAND meticpp_tests)