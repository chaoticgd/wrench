# CMake toolchain file for building with clang instead of gcc on Linux.
# To use: cmake -B bin -DCMAKE_TOOLCHAIN_FILE=clang.cmake

set(CMAKE_C_COMPILER "clang")
set(CMAKE_CXX_COMPILER "clang++")
