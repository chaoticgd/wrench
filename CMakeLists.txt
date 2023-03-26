cmake_minimum_required(VERSION 3.13)
project(wrench)

include_directories(src)
include_directories(thirdparty)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
	# Without this, MSVC won't report the correct standards version via the
	# __cplusplus macro. See:
	#  https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
	set(CMAKE_CXX_FLAGS "/Zc:__cplusplus")
	
	# Disables MSVC warnings for usage of insecure stdlib functions
	# like fopen instead of fopen_s.
	add_definitions(/D "_CRT_SECURE_NO_WARNINGS")
endif()

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-tree build detected. You should do an out-of-tree build instead:\n\tcmake -S . -B bin/")
endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})
include_directories(thirdparty/glm)
include_directories(thirdparty/libpng)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/libpng)

add_subdirectory(src)

if(UNIX)
	add_compile_options(-Wall -Wno-sign-compare -g -O3)
endif()

if(MSVC)
	# /W4 - enable all non-default-disabled warnings (-Wall equivalent)
	# /EHsc - needed for proper C++ exception handling
	# /MP - enables multi-process builds
	add_compile_options(/W4 /EHsc /MP)
endif()

add_subdirectory(thirdparty)
