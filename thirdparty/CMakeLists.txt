# cxxopts
add_subdirectory(cxxopts)

# GLAD
add_subdirectory(glad)
include_directories(glad/include)

# GLFW
add_subdirectory(glfw)
include_directories(glfw/include)

# GLM
add_subdirectory(glm)

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
add_subdirectory(zlib)
include_directories(zlib)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/zlib)
set(ZLIB_FOUND TRUE)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/zlib)

# libpng
option(PNG_BUILD_ZLIB "" ON)
option(PNG_SHARED "" OFF)
option(PNG_STATIC "" ON)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)
set(SKIP_INSTALL_EXPORT ON FORCE)
add_subdirectory(libpng)

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
if(WIN32)
	add_custom_target(copy_zlib_hack
		COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:zlibstatic>" "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib"
		DEPENDS zlibstatic
	)
	set(ZLIB_LIBRARY "${CMAKE_CURRENT_BINARY_DIR}/zlib.lib")

endif()
add_subdirectory(libzip)
if(WIN32)
	add_dependencies(zip copy_zlib_hack)
endif()
