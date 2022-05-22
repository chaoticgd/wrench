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

#include "level_textures.h"

#include <core/png.h>
#include <pakrac/level/level_core.h> // build_or_root_from_level_core_asset

static void write_nonshared_texture_data(OutputStream& data, std::vector<LevelTexture>& textures, Game game);

extern const char* DL_FX_TEXTURE_NAMES[98];

void unpack_level_textures(CollectionAsset& dest, const u8 indices[16], const std::vector<TextureEntry>& textures, InputStream& data, InputStream& gs_ram, Game game) {
	for(s32 i = 0; i < 16; i++) {
		if(indices[i] != 0xff) {
			const TextureEntry& texture = textures.at(indices[i]);
			unpack_level_texture(dest.child<TextureAsset>(i), texture, data, gs_ram, game, i);
		} else {
			break;
		}
	}
}

void unpack_level_texture(TextureAsset& dest, const TextureEntry& entry, InputStream& data, InputStream& gs_ram, Game game, s32 i) {
	std::vector<u8> pixels = data.read_multiple<u8>(entry.data_offset, entry.width * entry.height);
	std::vector<u32> palette = gs_ram.read_multiple<u32>(entry.palette * 0x100, 256);
	Texture texture = Texture::create_8bit_paletted(entry.width, entry.height, pixels, palette);
	
	texture.multiply_alphas();
	texture.swizzle_palette();
	if(game == Game::DL) {
		texture.swizzle();
	}
	
	auto [stream, ref] = dest.file().open_binary_file_for_writing(stringf("%d.png", i));
	write_png(*stream, texture);
	dest.set_src(ref);
}

SharedLevelTextures read_level_textures(CollectionAsset& tfrag_textures, CollectionAsset& mobies, CollectionAsset& ties, CollectionAsset& shrubs) {
	SharedLevelTextures shared;
	
	shared.tfrag_range.table = TFRAG_TEXTURE_TABLE;
	shared.tfrag_range.begin = shared.textures.size();
	for(s32 i = 0; i < 1024; i++) {
		if(tfrag_textures.has_child(i)) {
			TextureAsset& asset = tfrag_textures.get_child(i).as<TextureAsset>();
			auto stream = asset.file().open_binary_file_for_reading(asset.src());
			shared.textures.emplace_back(LevelTexture{read_png(*stream)});
		} else {
			shared.textures.emplace_back();
		}
	}
	shared.tfrag_range.end = shared.textures.size();
	
	shared.moby_range.table = MOBY_TEXTURE_TABLE;
	shared.moby_range.begin = shared.textures.size();
	mobies.for_each_logical_child_of_type<MobyClassAsset>([&](MobyClassAsset& cls) {
		CollectionAsset& textures = cls.get_materials();
		for(s32 i = 0; i < 16; i++) {
			if(textures.has_child(i)) {
				TextureAsset& asset = textures.get_child(i).as<TextureAsset>();
				auto stream = asset.file().open_binary_file_for_reading(asset.src());
				shared.textures.emplace_back(LevelTexture{read_png(*stream)});
			} else {
				shared.textures.emplace_back();
			}
		}
	});
	shared.moby_range.end = shared.textures.size();
	
	shared.tie_range.table = TIE_TEXTURE_TABLE;
	shared.tie_range.begin = shared.textures.size();
	ties.for_each_logical_child_of_type<TieClassAsset>([&](TieClassAsset& cls) {
		CollectionAsset& textures = cls.get_textures();
		for(s32 i = 0; i < 16; i++) {
			if(textures.has_child(i)) {
				TextureAsset& asset = textures.get_child(i).as<TextureAsset>();
				auto stream = asset.file().open_binary_file_for_reading(asset.src());
				shared.textures.emplace_back(LevelTexture{read_png(*stream)});
			} else {
				shared.textures.emplace_back();
			}
		}
	});
	shared.tie_range.end = shared.textures.size();
	
	shared.shrub_range.table = SHRUB_TEXTURE_TABLE;
	shared.shrub_range.begin = shared.textures.size();
	shrubs.for_each_logical_child_of_type<ShrubClassAsset>([&](ShrubClassAsset& cls) {
		CollectionAsset& textures = cls.get_textures();
		for(s32 i = 0; i < 16; i++) {
			if(textures.has_child(i)) {
				TextureAsset& asset = textures.get_child(i).as<TextureAsset>();
				auto stream = asset.file().open_binary_file_for_reading(asset.src());
				shared.textures.emplace_back(LevelTexture{read_png(*stream)});
			} else {
				shared.textures.emplace_back();
			}
		}
	});
	shared.shrub_range.end = shared.textures.size();
	
	return shared;
}

s64 write_shared_level_textures(OutputStream& data, OutputStream& gs, std::vector<GsRamEntry>& gs_table, std::vector<LevelTexture>& textures) {
	data.pad(0x100, 0);
	s64 ofs = data.tell();
	std::vector<u8> mipmap_data;
	for(LevelTexture& record : textures) {
		if(record.texture.has_value() && record.out_edge == -1) {
			const Texture& texture = *record.texture;
			
			if(record.palette_out_edge == -1) {
				gs.pad(0x100, 0);
				record.palette_offset = gs.tell();
				gs.write_v(texture.palette());
				gs_table.push_back({0, 0, 0, record.palette_offset, record.palette_offset});
			}
			
			mipmap_data.resize((texture.width * texture.height) / 16);
			for(s32 y = 0; y < texture.height / 4; y++) {
				for(s32 x = 0; x < texture.width / 4; x++) {
					mipmap_data[y * (texture.width / 4) + x] = texture.data[y * 4 * texture.width + x * 4];
				}
			}
			
			gs.pad(0x100, 0);
			record.mipmap_offset = gs.tell();
			gs.write_v(mipmap_data);
			
			GsRamEntry mipmap_entry;
			mipmap_entry.unknown_0 = 0x13;
			mipmap_entry.width = texture.width / 4;
			mipmap_entry.height = texture.height / 4;
			mipmap_entry.offset_1 = record.mipmap_offset;
			mipmap_entry.offset_2 = record.mipmap_offset;
			gs_table.push_back(mipmap_entry);
			
			data.pad(0x100, 0);
			record.texture_offset = data.tell();
			data.write_v(texture.data);
		}
	}
	return ofs;
}

ArrayRange write_level_texture_table(OutputStream& dest, std::vector<LevelTexture>& textures, LevelTextureRange range, s32 textures_base_offset) {
	dest.pad(0x10, 0);
	s32 table_offset = dest.tell();
	s32 table_count = 0;
	assert(range.begin <= range.end);
	for(s32 i = range.begin; i < range.end; i++) {
		LevelTexture* record = &textures.at(i);
		if(record->out_edge > -1) {
			record = &textures.at(record->out_edge);
		}
		// If there already exists an entry in the relevant table for the
		// texture, don't write another one.
		assert(range.table < 4);
		if(record->texture.has_value() && !record->indices[range.table].has_value()) {
			const Texture& texture = *record->texture;
			assert(record->texture_offset != -1);
			TextureEntry entry;
			entry.data_offset = record->texture_offset - textures_base_offset;
			entry.width = texture.width;
			entry.height = texture.height;
			entry.unknown_8 = 3;
			LevelTexture* palette_record = record;
			if(palette_record->palette_out_edge > -1) {
				palette_record = &textures.at(palette_record->palette_out_edge);
			}
			assert(palette_record->palette_offset != -1);
			entry.palette = palette_record->palette_offset / 0x100;
			entry.mipmap = record->mipmap_offset / 0x100;
			record->indices[range.table] = table_count;
			dest.write(entry);
			table_count++;
		}
	}
	return ArrayRange {table_count, table_offset};
}

s32 write_level_texture_indices(u8 dest[16], const std::vector<LevelTexture>& textures, s32 begin, s32 table) {
	for(s32 i = 0; i < 16; i++) {
		const LevelTexture* record = &textures.at(begin + i);
		if(record->texture.has_value()) {
			if(record->out_edge > -1) {
				record = &textures[record->out_edge];
			}
			assert(record->indices[table].has_value());
			verify(*record->indices[table] < 0xff, "Too many textures.\n");
			dest[i] = *record->indices[table];
		} else {
			for(s32 j = i; j < 16; j++) {
				dest[j] = 0xff;
				break;
			}
		}
	}
	return 16;
}

// *****************************************************************************

packed_struct(PartDefsHeader,
	/* 0x0 */ s32 particle_count;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 indices_offset;
	/* 0xc */ s32 indices_size;
)

void unpack_particle_textures(CollectionAsset& dest, InputStream& defs, std::vector<ParticleTextureEntry>& entries, InputStream& bank, Game game) {
	auto header = defs.read<PartDefsHeader>(0);
	auto offsets = defs.read_multiple<s32>(0x10, header.particle_count);
	auto indices = defs.read_multiple<u8>(header.indices_offset, header.indices_size);
	for(s32 part = 0; part < (s32) offsets.size(); part++) {
		if(offsets[part] == 0) {
			continue;
		}
		
		s32 begin = offsets[part] - header.indices_offset;
		s32 end = header.indices_size;
		for(s32 j = part + 1; j < (s32) offsets.size(); j++) {
			if(offsets[j] != 0) {
				end = offsets[j] - header.indices_offset;
				break;
			}
		}
		assert(begin >= 0 && end >= begin);
		CollectionAsset& file = dest.switch_files(stringf("particle_textures/%d/particle%d.asset", part, part));
		CollectionAsset& part_asset = file.child<CollectionAsset>(part);
		for(s32 frame = begin; frame < end; frame++) {
			u8 index = indices[frame];
			ParticleTextureEntry entry = entries.at(index);
			std::vector<u8> data = bank.read_multiple<u8>(entry.texture, entry.side * entry.side);
			std::vector<u32> palette = bank.read_multiple<u32>(entry.palette, 256);
			Texture texture = Texture::create_8bit_paletted(entry.side, entry.side, data, palette);
			
			if(game == Game::DL) {
				texture.swizzle();
			}
			texture.swizzle_palette();
			texture.multiply_alphas();
			
			TextureAsset& asset = part_asset.child<TextureAsset>(frame - begin);
			auto [stream, ref] = asset.file().open_binary_file_for_writing(stringf("%d.png", frame - begin));
			write_png(*stream, texture);
			asset.set_src(ref);
		}
	}
}

std::tuple<ArrayRange, s32, s32> pack_particle_textures(OutputStream& index, OutputStream& data, CollectionAsset& particles, Game game) {
	data.pad(0x100, 0);
	s64 particles_base = data.tell();
	
	s32 particle_count = 0;
	
	std::vector<LevelTexture> textures;
	std::map<s32, std::pair<s32, s32>> ranges;
	for(s32 i = 0; i < 1024; i++) {
		if(particles.has_child(i)) {
			CollectionAsset& particle = particles.get_child(i).as<CollectionAsset>();
			s32 begin = (s32) textures.size();
			for(s32 j = 0; j < 1024; j++) {
				if(particle.has_child(j)) {
					TextureAsset& asset = particle.get_child(j).as<TextureAsset>();
					auto stream = asset.file().open_binary_file_for_reading(asset.src());
					Opt<Texture> texture = read_png(*stream);
					if(texture.has_value()) {
						if(game == Game::DL) {
							texture->swizzle();
						}
						texture->swizzle_palette();
						texture->divide_alphas();
					}
					textures.emplace_back(LevelTexture{texture});
				} else {
					break;
				}
			}
			s32 end = (s32) textures.size();
			ranges[i] = {begin, end};
		} else {
			particle_count = i;
			continue;
		}
	}
	
	verify(textures.size() < 0x100, "Too many particle textures.");
	
	write_nonshared_texture_data(data, textures, game);
	
	// Write out the texture table.
	ArrayRange range;
	range.count = textures.size();
	range.offset = index.tell();
	index.pad(0x10, 0);
	s32 i = 0;
	for(LevelTexture& record : textures) {
		if(record.out_edge > -1) {
			continue;
		}
		
		LevelTexture* palette_record = &record;
		if(palette_record->palette_out_edge > -1) {
			palette_record = &textures[palette_record->palette_out_edge];
		}
		
		ParticleTextureEntry entry;
		entry.palette = palette_record->palette_offset - particles_base;
		entry.unknown_4 = 0;
		entry.texture = record.texture_offset - particles_base;
		verify(record.texture->width == record.texture->height,
			"Particle textures must be square.");
		entry.side = record.texture->width;
		index.write(entry);
		
		record.indices[0] = i++;
	}
	
	s64 frame_count = 0;
	
	particle_count = 0x81;
	
	// Write out the particle defs.
	index.pad(0x10, 0);
	s64 defs_base = index.alloc<PartDefsHeader>();
	PartDefsHeader defs_header = {0};
	defs_header.particle_count = particle_count;
	defs_header.indices_size = (s32) textures.size();
	
	s64 offsets_base = index.alloc_multiple<s32>(particle_count);
	index.pad(0x10, 0);
	defs_header.indices_offset = (s32) (index.tell() - defs_base);
	
	for(auto [particle, range] : ranges) {
		index.write<s32>(offsets_base + particle * 4, defs_header.indices_offset + range.first);
		for(s32 i = range.first; i < range.second; i++) {
			LevelTexture* texture = &textures[i];
			if(texture->out_edge > -1) {
				texture = &textures[texture->out_edge];
			}
			
			assert(texture->indices[0].has_value());
			index.write<u8>(*texture->indices[0]);
		}
	}
	
	index.write(defs_base, defs_header);
	
	return {range, defs_base, particles_base};
}

void unpack_fx_textures(LevelCoreAsset& core, const std::vector<FxTextureEntry>& entries, InputStream& fx_bank, Game game) {
	CollectionAsset& local_fx_textures = core.switch_files("fx_textures/fx_textures.asset").local_fx_textures();
	CollectionAsset& common_fx_textures = build_or_root_from_level_core_asset(core).switch_files("/fx_textures/fx_textures.asset").fx_textures();
	
	ReferenceAsset& common_fx_ref = core.child<ReferenceAsset>("common_fx_textures");
	common_fx_ref.set_asset(common_fx_textures.reference());
	
	std::vector<Texture> textures;
	for(size_t i = 0; i < entries.size(); i++) {
		const FxTextureEntry& entry = entries[i];
		std::vector<u32> palette = fx_bank.read_multiple<u32>(entry.palette, 256);
		std::vector<u8> pixels = fx_bank.read_multiple<u8>(entry.texture, entry.width * entry.height);
		Texture texture = Texture::create_8bit_paletted(entry.width, entry.height, pixels, palette);
	
		if(game == Game::DL) {
			texture.swizzle();
		}
		texture.swizzle_palette();
		texture.multiply_alphas();
		
		std::string name;
		switch(game) {
			case Game::DL: {
				if(i < ARRAY_SIZE(DL_FX_TEXTURE_NAMES)) {
					name = DL_FX_TEXTURE_NAMES[i];
				}
				break;
			}
		}
		
		CollectionAsset* collection;
		if(!name.empty()) {
			collection = &common_fx_textures;
		} else {
			name = stringf("%d", i);
			collection = &local_fx_textures;
		}
		
		TextureAsset& asset = collection->child<TextureAsset>(i);
		auto [stream, ref] = asset.file().open_binary_file_for_writing(name + ".png");
		write_png(*stream, texture);
		asset.set_src(ref);
	}
}

std::tuple<ArrayRange, s32> pack_fx_textures(OutputStream& index, OutputStream& data, CollectionAsset& common_fx, CollectionAsset& local_fx, Game game) {
	data.pad(0x100, 0);
	s64 fx_base = data.tell();
	
	std::vector<LevelTexture> textures;
	for(s32 i = 0; i < 1024; i++) {
		TextureAsset* asset;
		if(local_fx.has_child(i)) {
			asset = &local_fx.get_child(i).as<TextureAsset>();
		} else if(common_fx.has_child(i)) {
			asset = &common_fx.get_child(i).as<TextureAsset>();
		} else {
			break;
		}
		
		auto stream = asset->file().open_binary_file_for_reading(asset->src());
		Opt<Texture> texture = read_png(*stream);
		if(texture.has_value()) {
			if(game == Game::DL) {
				texture->swizzle();
			}
			texture->swizzle_palette();
			texture->divide_alphas();
		}
		textures.emplace_back(LevelTexture{texture});
	}
	
	write_nonshared_texture_data(data, textures, game);
	
	ArrayRange range;
	range.count = textures.size();
	range.offset = index.tell();
	index.pad(0x10, 0);
	for(LevelTexture& texture : textures) {
		LevelTexture* data_texture = &texture;
		if(data_texture->out_edge > -1) {
			data_texture = &textures[data_texture->out_edge];
		}
		
		LevelTexture* palette_texture = data_texture;
		if(palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		FxTextureEntry entry = {-1, -1, -1, -1};
		if(data_texture->texture) {
			entry.palette = palette_texture->palette_offset - fx_base;
			entry.texture = data_texture->texture_offset - fx_base;
			entry.width = data_texture->texture->width;
			entry.height = data_texture->texture->height;
		}
		index.write(entry);
	}
	
	return {range, fx_base};
}

static void write_nonshared_texture_data(OutputStream& data, std::vector<LevelTexture>& textures, Game game) {
	deduplicate_level_textures(textures);
	deduplicate_level_palettes(textures);
	
	for(LevelTexture& record : textures) {
		if(record.out_edge == -1) {
			data.pad(0x100, 0);
			if(record.palette_out_edge == -1) {
				record.palette_offset = data.tell();
				data.write_v(record.texture->palette());
			}
			data.pad(0x100, 0);
			record.texture_offset = data.tell();
			data.write_v(record.texture->data);
		}
	}
}

// *****************************************************************************

void deduplicate_level_textures(std::vector<LevelTexture>& textures) {
	std::vector<size_t> mapping;
	for(size_t i = 0; i < textures.size(); i++) {
		if(textures[i].texture.has_value()) {
			mapping.push_back(i);
		}
	}
	
	if(mapping.size() < 1) {
		return;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return *textures[lhs].texture < *textures[rhs].texture;
	});
	
	std::vector<size_t> group{mapping[0]};
	auto merge_group = [&]() {
		size_t lowest = SIZE_MAX;
		for(size_t index : group) {
			lowest = std::min(lowest, index);
		}
		assert(lowest != SIZE_MAX);
		for(size_t index : group) {
			if(index != lowest) {
				textures[index].out_edge = lowest;
			}
		}
	};
	
	for(size_t i = 1; i < mapping.size(); i++) {
		LevelTexture& last = textures[mapping[i - 1]];
		LevelTexture& cur = textures[mapping[i]];
		if(!(*last.texture == *cur.texture)) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if(group.size() > 0) {
		merge_group();
	}
}

void deduplicate_level_palettes(std::vector<LevelTexture>& textures) {
	std::vector<size_t> mapping;
	for(size_t i = 0; i < textures.size(); i++) {
		if(textures[i].texture.has_value() && textures[i].out_edge == -1) {
			mapping.push_back(i);
		}
	}
	
	if(mapping.size() < 1) {
		return;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return textures[lhs].texture->palette() < textures[rhs].texture->palette();
	});
	
	std::vector<size_t> group{mapping[0]};
	auto merge_group = [&]() {
		size_t lowest = SIZE_MAX;
		for(size_t index : group) {
			lowest = std::min(lowest, index);
		}
		assert(lowest != SIZE_MAX);
		for(size_t index : group) {
			if(index != lowest) {
				textures[index].palette_out_edge = lowest;
			}
		}
	};
	
	for(size_t i = 1; i < mapping.size(); i++) {
		LevelTexture& last = textures[mapping[i - 1]];
		LevelTexture& cur = textures[mapping[i]];
		if(!(last.texture->palette() == cur.texture->palette())) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if(group.size() > 0) {
		merge_group();
	}
}

const char* DL_FX_TEXTURE_NAMES[98] = {
	"lame_shadow",
	"ground_outer_reticule",
	"ground_inner_reticule",
	"center_screen_reticule1",
	"center_screen_reticule2",
	"generic_reticule",
	"cmd_attack",
	"cmd_defend",
	"cmd_emp",
	"cmd_shield",
	"cmd_mine",
	"jp_thrust_glow",
	"jp_thrust_highlight",
	"jp_thrust_fire",
	"lightning1",
	"engine",
	"glow_pill",
	"lens_flare_2",
	"ship_shadow",
	"sparkle",
	"wrench_blur",
	"suck_tornado",
	"white",
	"alpha_spark",
	"hologram",
	"tv_highlight",
	"tv_smallscan",
	"halo",
	"tv_scanlines",
	"tv_shine",
	"target_reticule",
	"cone_fire01_slim",
	"sandstorm",
	"progressbar_inner",
	"progressbar_outer",
	"ryno_reticule",
	"swingshot_reticule",
	"static",
	"blaster_reticule",
	"devastator_reticule",
	"triangle_reticule",
	"plasma_ball_core",
	"plasma_ball_aura",
	"plasma_lightning_bolt",
	"plasma_ball_flare",
	"plasma_ball_glow_ring",
	"steam_smoke_gas",
	"fork_lightning",
	"fork_lightning_glow_core",
	"starry_flash",
	"lava_glob",
	"main_ret1",
	"main_ret2",
	"main_ret3",
	"smoke_ring",
	"explotype1",
	"shockwave",
	"explosion",
	"plasma_shot",
	"heatmask2",
	"concrete",
	"shockwave01_keith",
	"muzzleflash1",
	"muzzleflash2",
	"streamer_keith",
	"muzzle_flower",
	"radialblur_sniper",
	"holoshield_base",
	"sniper_outer_reticule",
	"refractor_beam",
	"sniper_inner_reticule",
	"starburst1_keith",
	"starburst2_keith",
	"firecircle02_keith",
	"halfring_keith",
	"whirlpool_keith",
	"corona_keith",
	"pinch_alpha_mask",
	"duck_feather1",
	"duck_feather2",
	"cell_stream01",
	"cell_stream02",
	"bullet_trail_slim",
	"lightning02_keith",
	"lightning01_slim",
	"warpout_shockwave",
	"n60_reticule",
	"ground1_reticule",
	"ground2_reticule",
	"health_ball",
	"discblade_reticule",
	"shockblaster_reticule",
	"focus_ratchet_red",
	"focus_ratchet_blue",
	"focus_ratchet_red_dead",
	"focus_ratchet_blue_dead",
	"lock_on_reticule",
	"cracks",
};
