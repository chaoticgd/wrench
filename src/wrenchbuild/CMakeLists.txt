if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(wrenchbuild
	main.cpp
	tests.cpp
	asset_unpacker.cpp
	asset_packer.cpp
	classes/moby_class.cpp
	classes/shrub_class.cpp
	classes/tie_class.cpp
	common/binary_asset.cpp
	common/collection_asset.cpp
	common/elf_asset.cpp
	common/file_asset.cpp
	common/texture_asset.cpp
	common/flat_wad.cpp
	common/subtitles.cpp
	globals/armor_wad.cpp
	globals/audio_wad.cpp
	globals/bonus_wad.cpp
	globals/boot_wad.cpp
	globals/global_wad.cpp
	globals/hud_wad.cpp
	globals/irx_wad.cpp
	globals/misc_wad.cpp
	globals/mpeg_wad.cpp
	globals/online_wad.cpp
	globals/online_data_wad.cpp
	globals/space_wad.cpp
	level/level_wad.cpp
	level/level_data_wad.cpp
	level/level_core.cpp
	level/level_chunks.cpp
	level/level_classes.cpp
	level/level_textures.cpp
	level/level_audio_wad.cpp
	level/level_scene_wad.cpp
	level/collision_asset.cpp
	level/sky_asset.cpp
	level/tfrags_asset.cpp
	level/occlusion_asset.cpp
	level/instances_asset.cpp
)
target_link_libraries(wrenchbuild wrenchcore wrench_assetmgr wrenchiso wrenchengine wrenchinstancemgr wadinfo)
