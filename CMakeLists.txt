cmake_minimum_required(VERSION 3.13)
project(wrench CXX C)

option(PORTABLE_BUILD "Produce executables that are portable to other machines." ON)

enable_testing()

include_directories(src)
include_directories(thirdparty)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# libzip needs C extensions
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS ON)

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
	message(FATAL_ERROR "In-tree build detected. You should do an out-of-tree build instead:\n\tcmake -B bin/")
endif()

# Output all the binaries into the root of the build directory.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

include(cmake/version_finder.cmake)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

include_directories(${CMAKE_BINARY_DIR})
include_directories(thirdparty/glm)
include_directories(thirdparty/libpng)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/libpng)

include(cmake/wrench_compiler_flags.cmake)

add_subdirectory(src)

add_subdirectory(test)
add_subdirectory(thirdparty)

set(RELEASE_FILES
	$<TARGET_FILE:asset2json>
	$<TARGET_FILE:memmap>
	$<TARGET_FILE:vif>
	$<TARGET_FILE:wrenchbuild>
	$<TARGET_FILE:wrencheditor>
	$<TARGET_FILE:wrenchlauncher>
	$<TARGET_FILE:wrenchsaveeditor>
	$<TARGET_FILE:wrenchtrainer>
	$<TARGET_FILE:wrenchvis>
	"${CMAKE_BINARY_DIR}/editor.wad"
	"${CMAKE_BINARY_DIR}/gui.wad"
	"${CMAKE_BINARY_DIR}/launcher.wad"
	"${CMAKE_BINARY_DIR}/memcard.wad"
	"${CMAKE_BINARY_DIR}/trainer.wad"
)
set(RELEASE_DOCS_DIRECTORY "${CMAKE_SOURCE_DIR}/docs")
set(RELEASE_LICENSES_DIRECTORY "${CMAKE_SOURCE_DIR}/data/licenses")
set(RELEASE_UNDERLAY_DIRECTORY "${CMAKE_SOURCE_DIR}/data/underlay")
set(RELEASE_OVERLAY_DIRECTORY "${CMAKE_SOURCE_DIR}/data/overlay")
include(cmake/release_crafter.cmake)
