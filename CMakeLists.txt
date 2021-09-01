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
	src/editor/main.cpp
	src/buffer.cpp
	src/json.cpp
	src/instance.cpp
	src/level.cpp
	src/util.cpp
	src/mesh.cpp
	src/collada.cpp
	src/editor/app.cpp
	src/editor/command_line.cpp
	src/editor/stream.cpp
	src/editor/stacktrace.cpp
	src/editor/renderer.cpp
	src/editor/shaders.cpp
	src/editor/worker_logger.cpp
	src/editor/iso_stream.cpp
	src/editor/model.cpp
	src/editor/util.cpp
	src/editor/game_db.cpp
	src/editor/tools.cpp
	src/editor/icons.cpp
	src/editor/gui/gui.cpp
	src/editor/gui/window.cpp
	src/editor/gui/view_3d.cpp
	src/editor/gui/inspector.cpp
	src/editor/gui/imgui_impl_glfw.cpp
	src/editor/formats/level_impl.cpp
	src/editor/formats/texture.cpp
	src/editor/formats/fip.cpp
	src/editor/formats/bmp.cpp
	src/editor/formats/armor_archive.cpp
	src/editor/formats/game_model.cpp
	src/editor/formats/vif.cpp
	src/editor/formats/tfrag.cpp
	src/editor/formats/tcol.cpp
	src/editor/formats/shrub.cpp
	src/editor/formats/texture_archive.cpp
	src/editor/formats/model_utils.cpp
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	thirdparty/imgui/backends/imgui_impl_opengl3.cpp
	src/editor/md5.cpp
	src/lz/compression.cpp
)

add_executable(fip
	src/cli/fipcli.cpp
	src/editor/command_line.cpp
	src/editor/util.cpp
	src/editor/stream.cpp
	src/editor/stacktrace.cpp
	src/editor/formats/texture.cpp
	src/editor/formats/fip.cpp
	src/editor/formats/bmp.cpp
)

add_executable(lz
	src/lz/main.cpp
	src/lz/compression.cpp
	src/editor/util.cpp
	src/editor/command_line.cpp
	src/editor/stream.cpp
	src/editor/stacktrace.cpp
)

add_executable(vif
	src/cli/vifcli.cpp
	src/editor/stream.cpp
	src/editor/stacktrace.cpp
	src/editor/util.cpp
	src/editor/command_line.cpp
	src/editor/formats/vif.cpp
)

add_executable(iso
	src/iso/main.cpp
	src/iso/iso_filesystem.cpp
	src/iso/table_of_contents.cpp
	src/editor/stream.cpp
	src/editor/stacktrace.cpp
	src/editor/util.cpp
	src/editor/command_line.cpp
	src/buffer.cpp
	src/util.cpp
)

add_executable(memmap
	src/cli/memmap.c
)

add_executable(wad
	src/wad/main.cpp
	src/buffer.cpp
	src/json.cpp
	src/instance.cpp
	src/level.cpp
	src/util.cpp
	src/mesh.cpp
	src/collada.cpp
	src/wad/wad_file.cpp
	src/wad/gameplay.cpp
	src/wad/primary.cpp
	src/wad/assets.cpp
	src/wad/collision.cpp
	src/wad/texture.cpp
	src/wad/tests.cpp
	src/lz/compression.cpp
)

# This lets us avoid linking stuff for most of the CLI tools.
target_compile_definitions(wrench PRIVATE WRENCH_EDITOR=1)

if(UNIX)
	target_compile_options(wrench PRIVATE -Wall -O3)
	target_compile_options(fip PRIVATE -Wall -O3)
	target_compile_options(lz PRIVATE -Wall -O3)
	target_compile_options(vif PRIVATE -Wall -O3)
	target_compile_options(iso PRIVATE -Wall -O3)
	target_compile_options(wad PRIVATE -Wall -O3)
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
	target_compile_options(wad		PRIVATE /W4 /Zi /EHsc)
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
target_link_libraries(wad Threads::Threads)

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

add_subdirectory(src/version_check)
target_link_libraries(wrench version_check)
target_link_libraries(wad version_check)
