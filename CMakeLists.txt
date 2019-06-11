cmake_minimum_required (VERSION 2.8.11)
project(wrench)
include_directories(.)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(wrench
	src/main.cpp
	src/command_line.cpp
	src/window.cpp
	src/gui.cpp
	src/menu.cpp
	imgui/examples/imgui_impl_glfw.cpp
	imgui/examples/imgui_impl_opengl3.cpp)

make_directory(bin)
add_executable(bin/fip
	src/cli/fipcli.cpp
	src/command_line.cpp
	src/formats/fip.cpp)

make_directory(bin)
add_executable(bin/wad
	src/cli/wadcli.cpp
	src/command_line.cpp
	src/formats/wad.cpp)

# Boost
find_package(Boost 1.40 COMPONENTS program_options REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wrench ${Boost_LIBRARIES})
target_link_libraries(bin/fip ${Boost_LIBRARIES})
target_link_libraries(bin/wad ${Boost_LIBRARIES})

# OpenGL
find_package(OpenGL)
target_link_libraries(wrench ${OPENGL_LIBRARIES})

# GLEW
find_package(GLEW REQUIRED)
include_directories(${GLEW_INCLUDE_DIRS})
target_link_libraries(wrench ${GLEW_LIBRARIES})

# GLFW
find_package(glfw3 3.2 REQUIRED)
target_link_libraries(wrench glfw)
target_link_libraries(wrench ${OPENGL_gl_LIBRARY})

# Dear ImGui
include_directories(imgui)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW=1)
add_library(imgui STATIC
	imgui/imconfig.h
	imgui/imgui_demo.cpp
	imgui/imgui_draw.cpp
	imgui/imgui_internal.h
	imgui/imgui_widgets.cpp
	imgui/imgui.cpp
	imgui/imgui.h
	imgui/imstb_rectpack.h
	imgui/imstb_textedit.h
	imgui/imstb_truetype.h)
target_link_libraries(wrench imgui)
