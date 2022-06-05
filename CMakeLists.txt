cmake_minimum_required(VERSION 3.13)
project(wrench)

include_directories(src)
include_directories(thirdparty)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(MSVC)
	# Without this, MSVC won't report the correct standards version via the
	# __cplusplus macro. See:
	#  https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
	set(CMAKE_CXX_FLAGS "/Zc:__cplusplus")
	
	# Disables MSVC warnings for usage of insecure stdlib functions
	# like fopen instead of fopen_s.
	add_definitions(/D "_CRT_SECURE_NO_WARNINGS")
endif()

set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY $<1:${CMAKE_SOURCE_DIR}/bin>)

add_library(wrenchwtf STATIC
	src/wtf/wtf.c
	src/wtf/wtf_writer.cpp
)

add_library(wrenchcore STATIC
	src/core/buffer.cpp
	src/core/json.cpp
	src/core/instance.cpp
	src/core/level.cpp
	src/core/util.cpp
	src/core/mesh.cpp
	src/core/collada.cpp
	src/core/timer.cpp
	src/core/png.cpp
	src/core/texture.cpp
	src/core/vif.cpp
	src/core/filesystem.cpp
	src/core/stream.cpp
	src/core/memory_profiler.cpp
	thirdparty/md5.cpp
)
target_link_libraries(wrenchcore wrenchwtf)

add_executable(asset_codegen
	src/asset_codegen.cpp
)
target_link_libraries(asset_codegen wrenchwtf)

add_executable(asset_docgen
	src/asset_docgen.cpp
)
target_link_libraries(asset_docgen wrenchwtf)

add_custom_command(
	COMMAND asset_codegen src/assetmgr/asset_schema.wtf src/assetmgr/_generated_asset_types.inl
	DEPENDS asset_codegen src/assetmgr/asset_schema.wtf
	OUTPUT src/assetmgr/_generated_asset_types.inl
	COMMENT "Generating asset types"
)

add_custom_command(
	COMMAND asset_docgen src/assetmgr/asset_schema.wtf docs/asset_reference.md
	DEPENDS asset_docgen src/assetmgr/asset_schema.wtf
	OUTPUT docs/asset_reference.md
	COMMENT "Generating asset reference"
)

add_custom_target(
	asset_docgen_target ALL
	DEPENDS docs/asset_reference.md
)

add_library(wrench_assetmgr
	src/assetmgr/asset_util.cpp
	src/assetmgr/asset.cpp
	src/assetmgr/asset_types.cpp
	src/assetmgr/packed_asset_bank.cpp
	src/assetmgr/_generated_asset_types.inl
)
target_link_libraries(wrench_assetmgr wrenchcore)

add_subdirectory(src/toolwads)

add_library(wrenchgui
	src/gui/gui.cpp
	src/gui/config.cpp
	src/gui/about.cpp
	src/gui/settings.cpp
	src/gui/book.cpp
)
target_link_libraries(wrenchgui wrenchcore wrenchengine wadinfo)

add_executable(wrenchlauncher
	src/launcher/main.cpp
	src/launcher/global_state.cpp
	src/launcher/oobe.cpp
)
target_link_libraries(wrenchlauncher wrenchgui wrench_assetmgr)

add_executable(wrenchedit
	src/editor/main.cpp
	src/editor/app.cpp
	src/editor/command_line.cpp
	src/editor/renderer.cpp
	src/editor/shaders.cpp
	src/editor/worker_logger.cpp
	src/editor/mesh.cpp
	src/editor/util.cpp
	src/editor/game_db.cpp
	src/editor/tools.cpp
	src/editor/icons.cpp
	src/editor/gui/gui.cpp
	src/editor/gui/window.cpp
	src/editor/gui/view_3d.cpp
	src/editor/gui/inspector.cpp
	src/editor/formats/level_impl.cpp
)
target_link_libraries(wrenchedit wrenchcore)

add_executable(vif
	src/vifcli.cpp
	src/editor/util.cpp
	src/editor/command_line.cpp
)
target_link_libraries(vif wrenchcore)

add_library(wrenchiso
	src/iso/iso_unpacker.cpp
	src/iso/iso_packer.cpp
	src/iso/iso_tools.cpp
	src/iso/iso_filesystem.cpp
	src/iso/table_of_contents.cpp
	src/iso/wad_identifier.cpp
)
target_link_libraries(wrenchiso wrenchcore wrench_assetmgr)

add_executable(memmap
	src/memmap.c
)

add_library(wrenchengine
	src/engine/collision.cpp
	src/engine/compression.cpp
	src/engine/moby.cpp
	src/engine/moby_mesh_exporter.cpp
	src/engine/moby_mesh_importer.cpp
	src/engine/moby_animation.cpp
)
target_link_libraries(wrenchengine wrenchcore)

add_executable(wrenchbuild
	src/wrenchbuild/main.cpp
	src/wrenchbuild/gameplay.cpp
	src/wrenchbuild/tests.cpp
	src/wrenchbuild/asset_unpacker.cpp
	src/wrenchbuild/asset_packer.cpp
	src/wrenchbuild/release.cpp
	src/wrenchbuild/classes/moby_class.cpp
	src/wrenchbuild/common/binary_asset.cpp
	src/wrenchbuild/common/collection_asset.cpp
	src/wrenchbuild/common/file_asset.cpp
	src/wrenchbuild/common/texture_asset.cpp
	src/wrenchbuild/common/flat_wad.cpp
	src/wrenchbuild/globals/armor_wad.cpp
	src/wrenchbuild/globals/audio_wad.cpp
	src/wrenchbuild/globals/bonus_wad.cpp
	src/wrenchbuild/globals/boot_wad.cpp
	src/wrenchbuild/globals/hud_wad.cpp
	src/wrenchbuild/globals/irx_wad.cpp
	src/wrenchbuild/globals/misc_wad.cpp
	src/wrenchbuild/globals/mpeg_wad.cpp
	src/wrenchbuild/globals/online_wad.cpp
	src/wrenchbuild/globals/online_data_wad.cpp
	src/wrenchbuild/globals/space_wad.cpp
	src/wrenchbuild/level/level_wad.cpp
	src/wrenchbuild/level/level_data_wad.cpp
	src/wrenchbuild/level/level_core.cpp
	src/wrenchbuild/level/level_classes.cpp
	src/wrenchbuild/level/level_textures.cpp
	src/wrenchbuild/level/level_audio_wad.cpp
	src/wrenchbuild/level/level_scene_wad.cpp
)
target_link_libraries(wrenchbuild wrenchcore wrench_assetmgr wrenchiso wrenchengine wadinfo)

add_executable(asset2json
	src/asset2json.cpp
)
target_link_libraries(asset2json wrenchcore)

if(UNIX)
	set(GCC_FLAGS -Wall -Wno-sign-compare -g)
	target_compile_options(wrenchcore PRIVATE ${GCC_FLAGS} -O3)
	target_compile_options(wrench_assetmgr PRIVATE ${GCC_FLAGS})
	target_compile_options(wrenchlauncher PRIVATE ${GCC_FLAGS})
	target_compile_options(wrenchedit PRIVATE ${GCC_FLAGS})
	target_compile_options(vif PRIVATE ${GCC_FLAGS})
	target_compile_options(wrenchiso PRIVATE ${GCC_FLAGS})
	target_compile_options(wrenchengine PRIVATE ${GCC_FLAGS} -O3)
	target_compile_options(wrenchbuild PRIVATE ${GCC_FLAGS} -O3)
	target_compile_options(asset2json PRIVATE ${GCC_FLAGS})
endif()

if (MSVC)
	# /W4 - enable all non-default-disabled warnings (-Wall equivalent)
	# /EHsc - needed for proper C++ exception handling
	# /MP - enables multi-process builds
	set(__MSVC_BUILD_OPTIONS__ /W4 /EHsc /MP)

	target_compile_options(wrenchcore PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrench_assetmgr PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrenchlauncher PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrenchedit PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(vif PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrenchiso PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrenchengine PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(wrenchbuild PRIVATE ${__MSVC_BUILD_OPTIONS__})
	target_compile_options(asset2json PRIVATE ${__MSVC_BUILD_OPTIONS__})
	unset(__MSVC_BUILD_OPTIONS__)
endif()

# std::filesystem
if(UNIX)
	target_link_libraries(wrenchcore stdc++fs)
	target_link_libraries(wrenchlauncher stdc++fs)
	target_link_libraries(wrenchedit stdc++fs)
	target_link_libraries(vif stdc++fs)
	target_link_libraries(wrenchiso stdc++fs)
	target_link_libraries(wrenchengine stdc++fs)
	target_link_libraries(wrenchbuild stdc++fs)
endif()

# cxxopts
add_subdirectory(thirdparty/cxxopts)
target_link_libraries(wrenchlauncher cxxopts)
target_link_libraries(wrenchedit cxxopts)
target_link_libraries(vif cxxopts)
target_link_libraries(wrenchiso cxxopts)

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(wrenchcore Threads::Threads)

# GLAD
add_subdirectory(thirdparty/glad)
target_link_libraries(wrenchlauncher glad)
target_link_libraries(wrenchedit glad)

# GLFW
add_subdirectory(thirdparty/glfw)
target_link_libraries(wrenchlauncher glfw)
target_link_libraries(wrenchedit glfw)

# GLM
add_subdirectory(thirdparty/glm)
target_link_libraries(wrenchlauncher glm)
target_link_libraries(wrenchedit glm)
include_directories(thirdparty/glm)

# Dear ImGui
include_directories(thirdparty/imgui)
add_definitions(-DIMGUI_IMPL_OPENGL_LOADER_GLAD=1)
add_library(imgui STATIC
	thirdparty/imgui/imconfig.h
	thirdparty/imgui/imgui_demo.cpp
	thirdparty/imgui/imgui_draw.cpp
	thirdparty/imgui/imgui_internal.h
	thirdparty/imgui/imgui_tables.cpp
	thirdparty/imgui/imgui_widgets.cpp
	thirdparty/imgui/imgui.cpp
	thirdparty/imgui/imgui.h
	thirdparty/imgui/imstb_rectpack.h
	thirdparty/imgui/imstb_textedit.h
	thirdparty/imgui/imstb_truetype.h
	thirdparty/imgui/misc/cpp/imgui_stdlib.cpp
	thirdparty/imgui/backends/imgui_impl_glfw.cpp
	thirdparty/imgui/backends/imgui_impl_opengl3.cpp
)
target_link_libraries(wrenchlauncher imgui)
target_link_libraries(wrenchedit imgui)

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
target_link_libraries(wrenchlauncher nativefiledialog)
target_link_libraries(wrenchedit nativefiledialog)
include_directories(thirdparty/nativefiledialog/src/include)

# libpng
include_directories(wrenchcore thirdparty/zlib thirdparty/libpng)
add_subdirectory(thirdparty/zlib)
option(PNG_BUILD_ZLIB "" ON)
option(PNG_SHARED "" OFF)
option(PNG_STATIC "" ON)
option(PNG_EXECUTABLES "" OFF)
option(PNG_TESTS "" OFF)
set(ZLIB_LIBRARY zlibstatic)
set(ZLIB_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/thirdparty/zlib)
set(SKIP_INSTALL_EXPORT ON FORCE)
add_subdirectory(thirdparty/libpng)
target_link_libraries(wrenchcore png_static zlibstatic)
