cmake_minimum_required(VERSION 2.8.11)
project(wrench)
include_directories(thirdparty)
include_directories(thirdparty/imgui)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
	# Without this, MSVC won't report the correct standards version via the
	# __cplusplus macro. See:
	#  https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
	set(CMAKE_CXX_FLAGS "/Zc:__cplusplus")
endif()

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
	src/iso_stream.cpp
	src/model.cpp
	src/util.cpp
	src/game_db.cpp
	src/tools.cpp
	src/icons.cpp
	src/lz/compression.cpp
	src/formats/level_impl.cpp
	src/formats/world.cpp
	src/formats/texture.cpp
	src/formats/fip.cpp
	src/formats/bmp.cpp
	src/formats/armor_archive.cpp
	src/formats/game_model.cpp
	src/formats/vif.cpp
	src/formats/tfrag.cpp
	src/formats/tcol.cpp
	src/formats/shrub.cpp
	src/formats/texture_archive.cpp
	src/formats/model_utils.cpp
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	src/imgui_impl_glfw.cpp
	thirdparty/imgui/backends/imgui_impl_opengl3.cpp
	src/md5.cpp
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

add_executable(lz
	src/lz/main.cpp
	src/lz/compression.cpp
	src/util.cpp
	src/command_line.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/win32_profiler/win32_profiler.cpp
)

add_executable(vif
	src/cli/vifcli.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/util.cpp
	src/command_line.cpp
	src/formats/vif.cpp
)

add_executable(iso
	src/iso/main.cpp
	src/iso/iso_filesystem.cpp
	src/iso/table_of_contents.cpp
	src/stream.cpp
	src/stacktrace.cpp
	src/util.cpp
	src/command_line.cpp
	src/win32_profiler/win32_profiler.cpp
)


add_executable(memmap
	src/cli/memmap.c
)

# This lets us avoid linking stuff for most of the CLI tools.
target_compile_definitions(wrench PRIVATE WRENCH_EDITOR=1)

if(UNIX)
	target_compile_options(wrench PRIVATE -Wall -O3)
	target_compile_options(fip PRIVATE -Wall -O3)
	target_compile_options(lz PRIVATE -Wall -O3)
	target_compile_options(vif PRIVATE -Wall -O3)
	target_compile_options(iso PRIVATE -Wall -O3)
endif()

if (MSVC)
	# /W4 - enable all non-default-disabled warnings (-Wall equivalent)
	# /Zi - emit debugging informations to PDB
	# /EHsc - needed for proper C++ exception handling
	target_compile_options(wrench	PRIVATE /W4 /Zi /EHsc)	
	target_compile_options(fip		PRIVATE /W4 /Zi /EHsc)
	target_compile_options(lz		PRIVATE /W4 /Zi /EHsc)
	target_compile_options(vif		PRIVATE /W4 /Zi /EHsc)
	target_compile_options(iso		PRIVATE /W4 /Zi /EHsc)
endif()

# std::filesystem
if(UNIX)
	target_link_libraries(wrench stdc++fs)
	target_link_libraries(fip stdc++fs)
	target_link_libraries(lz stdc++fs)
	target_link_libraries(vif stdc++fs)
	target_link_libraries(iso stdc++fs)
endif()

# cxxopts
add_subdirectory(thirdparty/cxxopts)
target_link_libraries(wrench cxxopts)
target_link_libraries(fip cxxopts)
target_link_libraries(lz cxxopts)
target_link_libraries(vif cxxopts)
target_link_libraries(iso cxxopts)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(wrench Threads::Threads)
target_link_libraries(lz Threads::Threads)

# GLAD
add_subdirectory(thirdparty/glad)
target_link_libraries(wrench glad)

# GLFW
add_subdirectory(thirdparty/glfw)
target_link_libraries(wrench glfw)

# GLM
add_subdirectory(thirdparty/glm)
target_link_libraries(wrench glm)
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

# JSON
include_directories(thirdparty/json/single_include)

# Native File Dialog
if(WIN32)
	add_library(nativefiledialog STATIC
		thirdparty/nativefiledialog/src/nfd_common.c
		thirdparty/nativefiledialog/src/nfd_win.cpp
	)
else()
	add_library(nativefiledialog STATIC
		thirdparty/nativefiledialog/src/nfd_common.c
		thirdparty/nativefiledialog/src/nfd_zenity.c
	)
endif()
target_link_libraries(wrench nativefiledialog)
include_directories(thirdparty/nativefiledialog/src/include)
