cmake_minimum_required(VERSION 2.8.11)
project(wrench)
include_directories(thirdparty)
include_directories(thirdparty/imgui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)

add_executable(wrench
	src/main.cpp
	src/app.cpp
	src/command_line.cpp
	src/gui.cpp
	src/view_3d.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/window.cpp
	src/renderer.cpp
	src/shaders.cpp
	src/worker_logger.cpp
	src/project.cpp
	src/iso_stream.cpp
	src/model.cpp
	src/util.cpp
	src/game_db.cpp
	src/formats/wad.cpp
	src/formats/racpak.cpp
	src/formats/level_impl.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
	src/formats/armor_archive.cpp
	src/formats/game_model.cpp
	src/formats/vif.cpp
	src/formats/tfrag.cpp
	src/formats/toc.cpp
	src/formats/texture_archive.cpp
	src/formats/model_utils.cpp
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	src/imgui_impl_glfw.cpp
	thirdparty/imgui/examples/imgui_impl_opengl3.cpp
	src/md5.cpp
	src/tests/compression_test.cpp
	src/tests/world_segment_test.cpp
)

add_executable(fip
	src/cli/fipcli.cpp
	src/command_line.cpp
	src/util.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(wad
	src/cli/wadcli.cpp
	src/util.cpp
	src/command_line.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/formats/wad.cpp
)

add_executable(scan
	src/cli/scancli.cpp
	src/command_line.cpp
	src/util.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/formats/wad.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(pakrac
	src/cli/pakraccli.cpp
	src/util.cpp
	src/command_line.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/formats/racpak.cpp
	src/formats/wad.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(texturefinder
	src/cli/texturefindercli.cpp
	src/util.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/command_line.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
)

add_executable(vif
	src/cli/vifcli.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/util.cpp
	src/command_line.cpp
	src/formats/vif.cpp
)

add_executable(randomiser
	src/cli/randomisercli.cpp
	# TODO: Clean up depdendency graph.
	src/app.cpp
	src/command_line.cpp
	src/gui.cpp
	src/view_3d.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/window.cpp
	src/renderer.cpp
	src/shaders.cpp
	src/worker_logger.cpp
	src/project.cpp
	src/iso_stream.cpp
	src/model.cpp
	src/util.cpp
	src/game_db.cpp
	src/formats/wad.cpp
	src/formats/racpak.cpp
	src/formats/level_impl.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
	src/formats/armor_archive.cpp
	src/formats/game_model.cpp
	src/formats/vif.cpp
	src/formats/tfrag.cpp
	src/formats/toc.cpp
	src/formats/texture_archive.cpp
	src/formats/model_utils.cpp
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	src/imgui_impl_glfw.cpp
	thirdparty/imgui/examples/imgui_impl_opengl3.cpp
	src/md5.cpp
)

add_executable(toc
	src/cli/toccli.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/util.cpp
	src/command_line.cpp
	src/formats/toc.cpp
)

add_executable(matchtoc
	src/cli/matchtoccli.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/util.cpp
	src/command_line.cpp
	src/formats/toc.cpp
)

# This lets us avoid linking stuff for msot of the CLI tools.
target_compile_definitions(wrench PRIVATE WRENCH_EDITOR=1)
target_compile_definitions(randomiser PRIVATE WRENCH_EDITOR=1)

if(UNIX)
	target_compile_options(wrench PRIVATE -Wall -O3)
	target_compile_options(fip PRIVATE -Wall -O3)
	target_compile_options(wad PRIVATE -Wall -O3)
	target_compile_options(pakrac PRIVATE -Wall -O3)
	target_compile_options(texturefinder PRIVATE -Wall -O3)
	target_compile_options(vif PRIVATE -Wall -O3)
	target_compile_options(randomiser PRIVATE -Wall -O3)
	target_compile_options(toc PRIVATE -Wall -O3)
	target_compile_options(matchtoc PRIVATE -Wall -O3)
endif()

# std::filesystem
if(UNIX)
	target_link_libraries(wrench stdc++fs)
	target_link_libraries(fip stdc++fs)
	target_link_libraries(wad stdc++fs)
	target_link_libraries(scan stdc++fs)
	target_link_libraries(pakrac stdc++fs)
	target_link_libraries(texturefinder stdc++fs)
	target_link_libraries(vif stdc++fs)
	target_link_libraries(randomiser stdc++fs)
	target_link_libraries(matchtoc stdc++fs)
endif()

# cxxopts
add_subdirectory(thirdparty/cxxopts)
target_link_libraries(wrench cxxopts)
target_link_libraries(fip cxxopts)
target_link_libraries(wad cxxopts)
target_link_libraries(scan cxxopts)
target_link_libraries(pakrac cxxopts)
target_link_libraries(texturefinder cxxopts)
target_link_libraries(vif cxxopts)
target_link_libraries(randomiser cxxopts)
target_link_libraries(toc cxxopts)
target_link_libraries(matchtoc cxxopts)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(wrench Threads::Threads)
target_link_libraries(randomiser Threads::Threads)

# GLAD
add_subdirectory(thirdparty/glad)
target_link_libraries(wrench glad)
target_link_libraries(randomiser glad)

# GLFW
add_subdirectory(thirdparty/glfw)
target_link_libraries(wrench glfw)
target_link_libraries(randomiser glfw)

# GLM
add_subdirectory(thirdparty/glm)
target_link_libraries(wrench glm)
target_link_libraries(randomiser glm)
include_directories(thirdparty/glm)

# Dear ImGui
include_directories(thidparty/imgui)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD=1)
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
target_link_libraries(randomiser imgui)

# JSON
include_directories(thirdparty/json/single_include)

# ZipLib
include("cmake/ziplib.cmake")
target_link_libraries(wrench ziplib)
target_link_libraries(randomiser ziplib)
include_directories(SYSTEM thirdparty/ziplib/Source) # Suppress warnings for ZipLib headers.

# Source Map
# if(UNIX)
# 	add_custom_target(sourcemap
# 		COMMAND rm -f docs/dev/source_map.svg
# 		COMMAND python3 ./scripts/graphifier/graphify.py src docs/dev/# source_map.svg
# 	)
# 	add_dependencies(wrench sourcemap)
# endif()
