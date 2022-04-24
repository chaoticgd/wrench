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
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static SectorRange range(Sector32 offset, const std::set<s64>& end_sectors);
static void unpack_dl_level_scene_wad(LevelSceneWadAsset& dest, InputStream& src, Game game);
static void pack_dl_level_scene_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelSceneWadAsset& src, Game game);

on_load(LevelScene, []() {
	LevelSceneWadAsset::funcs.unpack_dl = wrap_wad_unpacker_func<LevelSceneWadAsset>(unpack_dl_level_scene_wad);
	
	LevelSceneWadAsset::funcs.pack_dl = wrap_wad_packer_func<LevelSceneWadAsset>(pack_dl_level_scene_wad);
})

packed_struct(DeadlockedSceneHeader,
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

packed_struct(DeadlockedLevelSceneWadHeader,
	/* 0x0 */ s32 header_size;
	/* 0x4 */ Sector32 sector;
	/* 0x8 */ DeadlockedSceneHeader scenes[30];
)

static void unpack_dl_level_scene_wad(LevelSceneWadAsset& dest, InputStream& src, Game game) {
	auto header = src.read<DeadlockedLevelSceneWadHeader>(0);
	
	std::set<s64> end_sectors;
	for(const DeadlockedSceneHeader& scene : header.scenes) {
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
		for(Sector32 chunk : scene.chunks) {
			end_sectors.insert(chunk.sectors);
		}
	}
	end_sectors.insert(Sector32::size_from_bytes(src.size()).sectors);
	
	CollectionAsset& scenes = dest.scenes();
	for(s32 i = 0; i < ARRAY_SIZE(header.scenes); i++) {
		SceneAsset& scene = scenes.child<SceneAsset>(i).switch_files();
		const DeadlockedSceneHeader& scene_header = header.scenes[i];
		unpack_asset(scene.speech_english_left(), src, range(scene_header.speech_english_left, end_sectors), game);
		unpack_asset(scene.speech_english_right(), src, range(scene_header.speech_english_right, end_sectors), game);
		unpack_asset(scene.subtitles(), src, scene_header.subtitles, game);
		unpack_asset(scene.speech_french_left(), src, range(scene_header.speech_french_left, end_sectors), game);
		unpack_asset(scene.speech_french_right(), src, range(scene_header.speech_french_right, end_sectors), game);
		unpack_asset(scene.speech_german_left(), src, range(scene_header.speech_german_left, end_sectors), game);
		unpack_asset(scene.speech_german_right(), src, range(scene_header.speech_german_right, end_sectors), game);
		unpack_asset(scene.speech_spanish_left(), src, range(scene_header.speech_spanish_left, end_sectors), game);
		unpack_asset(scene.speech_spanish_right(), src, range(scene_header.speech_spanish_right, end_sectors), game);
		unpack_asset(scene.speech_italian_left(), src, range(scene_header.speech_italian_left, end_sectors), game);
		unpack_asset(scene.speech_italian_right(), src, range(scene_header.speech_italian_right, end_sectors), game);
		unpack_compressed_asset(scene.moby_load(), src, scene_header.moby_load, game);
		CollectionAsset& chunks = scene.chunks().switch_files();
		for(s32 j = 0; j < ARRAY_SIZE(scene_header.chunks); j++) {
			if(scene_header.chunks[j].sectors > 0) {
				unpack_compressed_asset(chunks.child<BinaryAsset>(j), src, range(scene_header.chunks[j], end_sectors), game);
			}
		}
	}
}

static void pack_dl_level_scene_wad(OutputStream& dest, std::vector<u8>* header_dest, LevelSceneWadAsset& src, Game game) {
	s64 base = dest.tell();
	
	DeadlockedLevelSceneWadHeader header = {0};
	header.header_size = sizeof(DeadlockedLevelSceneWadHeader);
	dest.write(header);
	dest.pad(SECTOR_SIZE, 0);
	
	CollectionAsset& scenes = src.get_scenes();
	for(s32 i = 0; i < ARRAY_SIZE(header.scenes); i++) {
		if(scenes.has_child(i)) {
			DeadlockedSceneHeader& scene_header = header.scenes[i];
			SceneAsset& scene = scenes.get_child(i).as<SceneAsset>();
			scene_header.speech_english_left = pack_asset_sa<Sector32>(dest, scene.get_speech_english_left(), game, base);
			scene_header.speech_english_right = pack_asset_sa<Sector32>(dest, scene.get_speech_english_right(), game, base);
			scene_header.subtitles = pack_asset_sa<SectorRange>(dest, scene.get_subtitles(), game, base);
			scene_header.speech_french_left = pack_asset_sa<Sector32>(dest, scene.get_speech_french_left(), game, base);
			scene_header.speech_french_right = pack_asset_sa<Sector32>(dest, scene.get_speech_french_right(), game, base);
			scene_header.speech_german_left = pack_asset_sa<Sector32>(dest, scene.get_speech_german_left(), game, base);
			scene_header.speech_german_right = pack_asset_sa<Sector32>(dest, scene.get_speech_german_right(), game, base);
			scene_header.speech_spanish_left = pack_asset_sa<Sector32>(dest, scene.get_speech_spanish_left(), game, base);
			scene_header.speech_spanish_right = pack_asset_sa<Sector32>(dest, scene.get_speech_spanish_right(), game, base);
			scene_header.speech_italian_left = pack_asset_sa<Sector32>(dest, scene.get_speech_italian_left(), game, base);
			scene_header.speech_italian_right = pack_asset_sa<Sector32>(dest, scene.get_speech_italian_right(), game, base);
			scene_header.moby_load = pack_compressed_asset_sa<SectorRange>(dest, scene.get_moby_load(), game, base);
			pack_compressed_assets_sa(dest, ARRAY_PAIR(scene_header.chunks), scene.get_chunks(), game, base);
		}
	}
	
	dest.write(base, header);
	if(header_dest) {
		OutBuffer(*header_dest).write(0, header);
	}
}

static SectorRange range(Sector32 offset, const std::set<s64>& end_sectors) {
	auto end_sector = end_sectors.upper_bound(offset.sectors);
	verify(end_sector != end_sectors.end(), "Header references audio beyond end of file. The WAD file may be truncated.");
	return {offset, Sector32(*end_sector - offset.sectors)};
}
