cmake_minimum_required (VERSION 2.8.11)
project(wrench)

add_executable(wrench
	src/main.cpp
	src/command_line.cpp)

# Boost
find_package(Boost 1.40 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wrench LINK_PUBLIC ${Boost_LIBRARIES})
