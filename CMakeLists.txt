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

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
	message(FATAL_ERROR "In-tree build detected. You should do an out-of-tree build instead:\n\tcmake -S . -B bin/")
endif()

# pthreads
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(wrenchwtf STATIC
	src/wtf/wtf.c
	src/wtf/wtf_writer.cpp
)

add_library(wrenchcore STATIC
	src/core/buffer.cpp
	src/core/instance.cpp
	src/core/level.cpp
	src/core/util/basic_util.cpp
	src/core/util/error_util.cpp
	src/core/mesh.cpp
	src/core/collada.cpp
	src/core/timer.cpp
	src/core/png.cpp
	src/core/texture.cpp
	src/core/vif.cpp
	src/core/filesystem.cpp
	src/core/stream.cpp
	src/core/memory_profiler.cpp
	src/core/shell.cpp
	src/core/build_config.cpp
	src/core/mesh_graph.cpp
	src/core/tristrip.cpp
	src/core/material.cpp
	src/core/tristrip_packet.cpp
	src/core/elf.cpp
	thirdparty/md5.cpp
)
target_link_libraries(wrenchcore wrenchwtf)
target_link_libraries(wrenchcore Threads::Threads png_static zlibstatic)

add_executable(asset_codegen
	src/asset_codegen.cpp
)
target_link_libraries(asset_codegen wrenchwtf)

add_executable(asset_docgen
	src/asset_docgen.cpp
)
target_link_libraries(asset_docgen wrenchwtf)

add_custom_command(
	COMMAND asset_codegen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf ${CMAKE_BINARY_DIR}/_generated_asset_types.inl
	DEPENDS asset_codegen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_asset_types.inl
	COMMENT "Generating asset types"
)

add_custom_command(
	COMMAND asset_docgen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf ${CMAKE_SOURCE_DIR}/docs/asset_reference.md
	DEPENDS asset_docgen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf
	OUTPUT docs/asset_reference.md
	COMMENT "Generating asset reference"
)

include_directories(${CMAKE_BINARY_DIR})

add_custom_target(
	asset_docgen_target ALL
	DEPENDS docs/asset_reference.md
)

add_library(wrench_assetmgr
	src/assetmgr/game_info.cpp
	src/assetmgr/asset_util.cpp
	src/assetmgr/asset.cpp
	src/assetmgr/asset_types.cpp
	src/assetmgr/zipped_asset_bank.cpp
	src/assetmgr/material_asset.cpp
	${CMAKE_BINARY_DIR}/_generated_asset_types.inl
)
target_link_libraries(wrench_assetmgr wrenchcore zip)

add_subdirectory(src/toolwads)

add_library(wrenchgui
	src/gui/gui.cpp
	src/gui/config.cpp
	src/gui/about.cpp
	src/gui/settings_screen.cpp
	src/gui/book.cpp
	src/gui/command_output.cpp
	src/gui/build_settings.cpp
	src/gui/commands.cpp
	src/gui/shaders.cpp
	src/gui/render_mesh.cpp
)
target_include_directories(wrenchgui PUBLIC thirdparty/imgui)
target_include_directories(wrenchgui PUBLIC thirdparty/nativefiledialog/src/include)
target_link_libraries(wrenchgui wrenchcore wrenchengine wadinfo)
target_link_libraries(wrenchgui glfw glm glad imgui nativefiledialog)

add_executable(wrenchlauncher
	src/launcher/main.cpp
	src/launcher/global_state.cpp
	src/launcher/oobe.cpp
	src/launcher/mod_list.cpp
	src/launcher/game_list.cpp
	src/launcher/new_mod_screen.cpp
	src/launcher/image_viewer.cpp
)
target_link_libraries(wrenchlauncher wrenchgui wrench_assetmgr)

add_executable(wrencheditor
	src/editor/main.cpp
	src/editor/app.cpp
	src/editor/command_line.cpp
	src/editor/util.cpp
	src/editor/tools.cpp
	src/editor/renderer.cpp
	src/editor/level.cpp
	src/editor/editor.cpp
	src/editor/gui/editor_gui.cpp
	src/editor/gui/view_3d.cpp
	src/editor/gui/inspector.cpp
	src/editor/gui/asset_selector.cpp
)
target_link_libraries(wrencheditor wrenchcore wrenchgui wrench_assetmgr cxxopts)

add_executable(vif
	src/vifcli.cpp
	src/editor/util.cpp
	src/editor/command_line.cpp
)
target_link_libraries(vif wrenchcore cxxopts)

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
	src/engine/gameplay.cpp
	src/engine/moby.cpp
	src/engine/moby_mesh_exporter.cpp
	src/engine/moby_mesh_importer.cpp
	src/engine/moby_animation.cpp
	src/engine/shrub.cpp
	src/engine/sky.cpp
	src/engine/tfrag.cpp
	src/engine/tie.cpp
	src/engine/vag.cpp
)
target_link_libraries(wrenchengine wrenchcore)

add_executable(wrenchbuild
	src/wrenchbuild/main.cpp
	src/wrenchbuild/tests.cpp
	src/wrenchbuild/asset_unpacker.cpp
	src/wrenchbuild/asset_packer.cpp
	src/wrenchbuild/release.cpp
	src/wrenchbuild/classes/moby_class.cpp
	src/wrenchbuild/classes/shrub_class.cpp
	src/wrenchbuild/classes/tie_class.cpp
	src/wrenchbuild/common/binary_asset.cpp
	src/wrenchbuild/common/collection_asset.cpp
	src/wrenchbuild/common/file_asset.cpp
	src/wrenchbuild/common/texture_asset.cpp
	src/wrenchbuild/common/flat_wad.cpp
	src/wrenchbuild/common/subtitles.cpp
	src/wrenchbuild/globals/armor_wad.cpp
	src/wrenchbuild/globals/audio_wad.cpp
	src/wrenchbuild/globals/bonus_wad.cpp
	src/wrenchbuild/globals/boot_wad.cpp
	src/wrenchbuild/globals/global_wad.cpp
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
	src/wrenchbuild/level/collision_asset.cpp
	src/wrenchbuild/level/sky_asset.cpp
	src/wrenchbuild/level/tfrags_asset.cpp
)
target_link_libraries(wrenchbuild wrenchcore wrench_assetmgr wrenchiso wrenchengine wadinfo)

add_executable(asset2json
	src/asset2json.cpp
)
target_include_directories(asset2json PUBLIC thirdparty/json/single_include)
target_link_libraries(asset2json wrenchcore)

add_executable(unpackbin
	src/unpackbin.cpp
)
target_link_libraries(unpackbin wrenchcore)

if(UNIX)
	add_compile_options(-Wall -Wno-sign-compare -g -O3)
endif()

if(MSVC)
	# /W4 - enable all non-default-disabled warnings (-Wall equivalent)
	# /EHsc - needed for proper C++ exception handling
	# /MP - enables multi-process builds
	add_compile_options(/W4 /EHsc /MP)
endif()

add_subdirectory(thirdparty)
include_directories(thirdparty/glm)
include_directories(thirdparty/libpng)
include_directories(${CMAKE_BINARY_DIR}/thirdparty/libpng)
