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
include_directories(glm)

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

# libpng
add_subdirectory(zlib)
option(PNG_BUILD_ZLIB "" ON)
option(PNG_SHARED "" OFF)
option(PNG_STATIC "" ON)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/zlib)
set(SKIP_INSTALL_EXPORT ON FORCE)
add_subdirectory(libpng)
