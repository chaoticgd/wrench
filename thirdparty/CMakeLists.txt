if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

# GLAD
add_subdirectory(glad EXCLUDE_FROM_ALL)
include_directories(glad/include)

# GLFW
add_subdirectory(glfw EXCLUDE_FROM_ALL)
include_directories(glfw/include)

# GLM
add_subdirectory(glm EXCLUDE_FROM_ALL)

# Dear ImGui
include_directories(imgui)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD=1)
add_library(imgui STATIC
	imgui/imconfig.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_internal.h
	imgui/imgui_tables.cpp
	imgui/imgui_widgets.cpp
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h
	imgui/misc/cpp/imgui_stdlib.cpp
	imgui/backends/imgui_impl_glfw.cpp
	imgui/backends/imgui_impl_opengl3.cpp
)

# JSON
include_directories(json/single_include)

# Native File Dialog
if(WIN32)
	add_library(nativefiledialog STATIC
		nativefiledialog/src/nfd_common.c
		nativefiledialog/src/nfd_win.cpp
	)
else()
	add_library(nativefiledialog STATIC
		nativefiledialog/src/nfd_common.c
		nativefiledialog/src/nfd_zenity.c
	)
endif()
include_directories(nativefiledialog/src/include)

# zlib
set(SKIP_INSTALL_ALL ON)
add_subdirectory(zlib EXCLUDE_FROM_ALL)
include_directories(zlib)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/zlib)
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/zlib ${CMAKE_BINARY_DIR}/thirdparty/zlib)
# ZLIB_INCLUDE_DIRS is used by genout.cmake.in from libpng.
set(ZLIB_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/thirdparty/zlib ${CMAKE_BINARY_DIR}/thirdparty/zlib)
add_library(ZLIB::ZLIB ALIAS zlibstatic)
if(WIN32)
	# ZLIB_LIBRARY is used by libzip and must be set at configuration time hence
	# we copy the library file into a fixed location.
	add_custom_target(copy_zlib_lib_hack
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:zlibstatic>" "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib"
	)
	set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib")
endif()

# libpng
option(PNG_BUILD_ZLIB "" ON)
option(PNG_SHARED "" OFF)
option(PNG_STATIC "" ON)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)
set(SKIP_INSTALL_EXPORT ON FORCE)
add_subdirectory(libpng EXCLUDE_FROM_ALL)

# libzip
set(LIBZIP_DO_INSTALL OFF CACHE BOOL "")
set(ENABLE_COMMONCRYPTO OFF CACHE BOOL "")
set(ENABLE_GNUTLS OFF CACHE BOOL "")
set(ENABLE_MBEDTLS OFF CACHE BOOL "")
set(ENABLE_WINDOWS_CRYPTO OFF CACHE BOOL "")
set(ENABLE_OPENSSL OFF CACHE BOOL "")
set(ENABLE_BZIP2 OFF CACHE BOOL "")
set(ENABLE_LZMA OFF CACHE BOOL "")
set(ENABLE_ZSTD OFF CACHE BOOL "")
set(BUILD_TOOLS OFF CACHE BOOL "")
set(BUILD_REGRESS OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
set(BUILD_DOC OFF CACHE BOOL "")
add_subdirectory(libzip EXCLUDE_FROM_ALL)
if(WIN32)
	add_dependencies(zip copy_zlib_lib_hack)
endif()

# Catch2
add_subdirectory(catch2 EXCLUDE_FROM_ALL)

# ImGuizmo
add_library(imguizmo STATIC
	imguizmo/ImGuizmo.cpp
)

# PINE
add_subdirectory(pine EXLCUDE_FROM_ALL)
