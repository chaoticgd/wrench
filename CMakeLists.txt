cmake_minimum_required(VERSION 2.8.11)
project(wrench)
include_directories(thirdparty)
include_directories(thirdparty/imgui)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_executable(wrench
	src/main.cpp
	src/app.cpp
	src/command_line.cpp
	src/gui.cpp
	src/view_3d.cpp
	src/stream.cpp
	src/window.cpp
	src/renderer.cpp
	src/shaders.cpp
	src/texture.cpp
	src/worker_logger.cpp
	src/project.cpp
	src/iso_stream.cpp
	src/model.cpp
	src/util.cpp
	src/game_db.cpp
	src/formats/wad.cpp
	src/formats/racpak.cpp
	src/formats/level_impl.cpp
	src/formats/texture_impl.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
	src/formats/armor_archive.cpp
	src/formats/game_model.cpp
	src/formats/vif.cpp
	src/commands/translate_command.cpp
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	src/imgui_impl_glfw.cpp
	thirdparty/imgui/examples/imgui_impl_opengl3.cpp
	src/md5.cpp
	src/platform_linux.cpp
)

add_executable(fip
	src/cli/fipcli.cpp
	src/command_line.cpp
	src/stream.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(wad
	src/cli/wadcli.cpp
	src/command_line.cpp
	src/stream.cpp
	src/formats/wad.cpp
)

add_executable(scan
	src/cli/scancli.cpp
	src/command_line.cpp
	src/stream.cpp
	src/formats/wad.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(pakrac
	src/cli/pakraccli.cpp
	src/util.cpp
	src/command_line.cpp
	src/stream.cpp
	src/formats/racpak.cpp
	src/formats/wad.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(texturefinder
	src/cli/texturefindercli.cpp
	src/stream.cpp
	src/command_line.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(vif
	src/cli/vifcli.cpp
	src/stream.cpp
	src/util.cpp
	src/command_line.cpp
	src/formats/vif.cpp
)

if(MSVC)
	target_compile_options(wrench PRIVATE /W4 /WX)
	target_compile_options(fip PRIVATE /W4 /WX)
	target_compile_options(wad PRIVATE /W4 /WX)
	target_compile_options(scan PRIVATE /W4 /WX)
	target_compile_options(pakrac PRIVATE /W4 /WX)
	target_compile_options(texturefinder PRIVATE /W4 /WX)
	target_compile_options(vif PRIVATE /W4 /WX)
else()
	target_compile_options(wrench PRIVATE -Wall -O3)
	target_compile_options(fip PRIVATE -Wall -O3)
	target_compile_options(wad PRIVATE -Wall -O3)
	target_compile_options(pakrac PRIVATE -Wall -O3)
	target_compile_options(texturefinder PRIVATE -Wall -O3)
	target_compile_options(vif PRIVATE -Wall -O3)
endif()

# Boost
find_package(Boost 1.40 COMPONENTS program_options filesystem REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
target_link_libraries(wrench ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(fip ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(wad ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(scan ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(pakrac ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(texturefinder ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})
target_link_libraries(vif ${CMAKE_DL_LIBS} ${Boost_LIBRARIES})

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(wrench Threads::Threads)

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
include_directories(thidparty/imgui)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLEW=1)
add_library(imgui STATIC
	thirdparty/imgui/imconfig.h
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_internal.h
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui.h
	thirdparty/imgui/imstb_rectpack.h
	thirdparty/imgui/imstb_textedit.h
	thirdparty/imgui/imstb_truetype.h
)
target_link_libraries(wrench imgui)

# JSON
include_directories(thirdparty/json/single_include)

# ZipLib
include("cmake/ziplib.cmake")
target_link_libraries(wrench ziplib)
include_directories(SYSTEM thirdparty/ziplib/Source) # Suppress warnings for ZipLib headers.

# Source Map
add_custom_target(sourcemap
	COMMAND rm -f docs/dev/source_map.svg
	COMMAND python3 ./scripts/graphifier/graphify.py src docs/dev/source_map.svg
)
add_dependencies(wrench sourcemap)
