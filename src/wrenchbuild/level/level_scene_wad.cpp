/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#include <set>
#include <iso/table_of_contents.h>
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>

packed_struct(DlSceneHeader,
	/* 0x00 */ Sector32 speech_english_left;
	/* 0x04 */ Sector32 speech_english_right;
	/* 0x08 */ SectorRange subtitles;
	/* 0x10 */ Sector32 speech_french_left;
	/* 0x14 */ Sector32 speech_french_right;
	/* 0x18 */ Sector32 speech_german_left;
	/* 0x1c */ Sector32 speech_german_right;
	/* 0x20 */ Sector32 speech_spanish_left;
	/* 0x24 */ Sector32 speech_spanish_right;
	/* 0x28 */ Sector32 speech_italian_left;
	/* 0x2c */ Sector32 speech_italian_right;
	/* 0x30 */ SectorRange moby_load;
	/* 0x38 */ Sector32 chunks[69];
)

packed_struct(DlLevelSceneWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ DlSceneHeader scenes[30];
)

static void unpack_rac_level_scene_wad(
	LevelSceneWadAsset& dest,
	const RacLevelSceneWadHeader& header,
	InputStream& src,
	BuildConfig config);
static void pack_rac_level_scene_wad(
	OutputStream& dest,
	RacLevelSceneWadHeader& header,
	const LevelSceneWadAsset& src,
	BuildConfig config);
static void unpack_dl_level_scene_wad(
	LevelSceneWadAsset& dest,
	const DlLevelSceneWadHeader& header,
	InputStream& src,
	BuildConfig config);
static void pack_dl_level_scene_wad(
	OutputStream& dest,
	DlLevelSceneWadHeader& header,
	const LevelSceneWadAsset& src,
	BuildConfig config);
static SectorRange range(Sector32 offset, const std::set<s64>& end_sectors);

on_load(LevelScene, []() {
	LevelSceneWadAsset::funcs.unpack_rac1 = wrap_wad_unpacker_func<LevelSceneWadAsset, RacLevelSceneWadHeader>(unpack_rac_level_scene_wad, false);
	LevelSceneWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelSceneWadAsset, DlLevelSceneWadHeader>(unpack_dl_level_scene_wad, false);
	
	LevelSceneWadAsset::funcs.pack_rac1 = wrap_wad_packer_func<LevelSceneWadAsset, RacLevelSceneWadHeader>(pack_rac_level_scene_wad);
	LevelSceneWadAsset::funcs.pack_dl = wrap_wad_packer_func<LevelSceneWadAsset, DlLevelSceneWadHeader>(pack_dl_level_scene_wad);
})

static void unpack_rac_level_scene_wad(
	LevelSceneWadAsset& dest,
	const RacLevelSceneWadHeader& header,
	InputStream& src,
	BuildConfig config)
{
	
}

static void pack_rac_level_scene_wad(
	OutputStream& dest,
	RacLevelSceneWadHeader& header,
	const LevelSceneWadAsset& src,
	BuildConfig config)
{
	
}

static void unpack_dl_level_scene_wad(
	LevelSceneWadAsset& dest,
	const DlLevelSceneWadHeader& header,
	InputStream& src,
	BuildConfig config)
{
	std::set<s64> end_sectors;
	for (const DlSceneHeader& scene : header.scenes) {
		end_sectors.insert(scene.speech_english_left.sectors);
		end_sectors.insert(scene.speech_english_right.sectors);
		end_sectors.insert(scene.subtitles.offset.sectors);
		end_sectors.insert(scene.speech_french_left.sectors);
		end_sectors.insert(scene.speech_french_right.sectors);
		end_sectors.insert(scene.speech_german_left.sectors);
		end_sectors.insert(scene.speech_german_right.sectors);
		end_sectors.insert(scene.speech_spanish_left.sectors);
		end_sectors.insert(scene.speech_spanish_right.sectors);
		end_sectors.insert(scene.speech_italian_left.sectors);
		end_sectors.insert(scene.speech_italian_right.sectors);
		end_sectors.insert(scene.moby_load.offset.sectors);
		for (Sector32 chunk : scene.chunks) {
			end_sectors.insert(chunk.sectors);
		}
	}
	end_sectors.insert(Sector32::size_from_bytes(src.size()).sectors);
	
	CollectionAsset& scenes = dest.scenes();
	for (s32 i = 0; i < ARRAY_SIZE(header.scenes); i++) {
		SceneAsset& scene = scenes.foreign_child<SceneAsset>(stringf("scenes/%d/%d", i, i), false, i);
		const DlSceneHeader& scene_header = header.scenes[i];
		unpack_asset(scene.speech_english_left(), src, range(scene_header.speech_english_left, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_english_right(), src, range(scene_header.speech_english_right, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.subtitles(), src, scene_header.subtitles, config);
		unpack_asset(scene.speech_french_left(), src, range(scene_header.speech_french_left, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_french_right(), src, range(scene_header.speech_french_right, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_german_left(), src, range(scene_header.speech_german_left, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_german_right(), src, range(scene_header.speech_german_right, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_spanish_left(), src, range(scene_header.speech_spanish_left, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_spanish_right(), src, range(scene_header.speech_spanish_right, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_italian_left(), src, range(scene_header.speech_italian_left, end_sectors), config, FMT_BINARY_VAG);
		unpack_asset(scene.speech_italian_right(), src, range(scene_header.speech_italian_right, end_sectors), config, FMT_BINARY_VAG);
		unpack_compressed_asset(scene.moby_load(), src, scene_header.moby_load, config);
		CollectionAsset& chunks = scene.chunks(SWITCH_FILES);
		for (s32 j = 0; j < ARRAY_SIZE(scene_header.chunks); j++) {
			if (scene_header.chunks[j].sectors > 0) {
				unpack_compressed_asset(chunks.child<BinaryAsset>(j), src, range(scene_header.chunks[j], end_sectors), config);
			}
		}
	}
}

static void pack_dl_level_scene_wad(
	OutputStream& dest,
	DlLevelSceneWadHeader& header,
	const LevelSceneWadAsset& src,
	BuildConfig config)
{
	const CollectionAsset& scenes = src.get_scenes();
	for (s32 i = 0; i < ARRAY_SIZE(header.scenes); i++) {
		if (scenes.has_child(i)) {
			DlSceneHeader& scene_header = header.scenes[i];
			const SceneAsset& scene = scenes.get_child(i).as<SceneAsset>();
			scene_header.speech_english_left = pack_asset_sa<Sector32>(dest, scene.get_speech_english_left(), config, FMT_BINARY_VAG);
			scene_header.speech_english_right = pack_asset_sa<Sector32>(dest, scene.get_speech_english_right(), config, FMT_BINARY_VAG);
			scene_header.subtitles = pack_asset_sa<SectorRange>(dest, scene.get_subtitles(), config);
			scene_header.speech_french_left = pack_asset_sa<Sector32>(dest, scene.get_speech_french_left(), config, FMT_BINARY_VAG);
			scene_header.speech_french_right = pack_asset_sa<Sector32>(dest, scene.get_speech_french_right(), config, FMT_BINARY_VAG);
			scene_header.speech_german_left = pack_asset_sa<Sector32>(dest, scene.get_speech_german_left(), config, FMT_BINARY_VAG);
			scene_header.speech_german_right = pack_asset_sa<Sector32>(dest, scene.get_speech_german_right(), config, FMT_BINARY_VAG);
			scene_header.speech_spanish_left = pack_asset_sa<Sector32>(dest, scene.get_speech_spanish_left(), config, FMT_BINARY_VAG);
			scene_header.speech_spanish_right = pack_asset_sa<Sector32>(dest, scene.get_speech_spanish_right(), config, FMT_BINARY_VAG);
			scene_header.speech_italian_left = pack_asset_sa<Sector32>(dest, scene.get_speech_italian_left(), config, FMT_BINARY_VAG);
			scene_header.speech_italian_right = pack_asset_sa<Sector32>(dest, scene.get_speech_italian_right(), config, FMT_BINARY_VAG);
			scene_header.moby_load = pack_compressed_asset_sa<SectorRange>(dest, scene.get_moby_load(), config, "moby_load");
			pack_compressed_assets_sa(dest, ARRAY_PAIR(scene_header.chunks), scene.get_chunks(), config, "chunks");
		}
	}
}

static SectorRange range(Sector32 offset, const std::set<s64>& end_sectors)
{
	auto end_sector = end_sectors.upper_bound(offset.sectors);
	verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
	return {offset, Sector32(*end_sector - offset.sectors)};
}
