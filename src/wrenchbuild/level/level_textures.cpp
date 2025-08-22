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

#include <map>
#include <algorithm>

#include <core/png.h>
#include <assetmgr/material_asset.h>

static void write_nonshared_texture_data(
	OutputStream& data, std::vector<LevelTexture>& textures, Game game);

extern const char* GC_FX_TEXTURE_NAMES[63];
extern const char* UYA_FX_TEXTURE_NAMES[100];
extern const char* DL_FX_TEXTURE_NAMES[98];

void unpack_level_materials(
	CollectionAsset& dest,
	const u8 indices[16],
	const std::vector<TextureEntry>& textures,
	InputStream& data,
	InputStream& gs_ram,
	Game game,
	s32 moby_stash_addr)
{
	for (s32 i = 0; i < 16; i++) {
		if (indices[i] != 0xff) {
			const TextureEntry& texture = textures.at(indices[i]);
			unpack_level_material(dest.child<MaterialAsset>(i), texture, data, gs_ram, game, i, moby_stash_addr);
		} else {
			break;
		}
	}
}

void unpack_level_material(
	MaterialAsset& dest,
	const TextureEntry& entry,
	InputStream& data,
	InputStream& gs_ram,
	Game game,
	s32 i,
	s32 moby_stash_addr)
{
	std::vector<u8> pixels;
	if (moby_stash_addr > -1) {
		pixels = gs_ram.read_multiple<u8>(moby_stash_addr + entry.data_offset, entry.width * entry.height);
	} else {
		pixels = data.read_multiple<u8>(entry.data_offset, entry.width * entry.height);
	}
	std::vector<u32> palette = gs_ram.read_multiple<u32>(entry.palette * 0x100, 256);
	Texture texture = Texture::create_8bit_paletted(entry.width, entry.height, pixels, palette);
	
	texture.multiply_alphas();
	texture.swizzle_palette();
	if (game == Game::DL) {
		texture.swizzle();
	}
	
	auto [stream, ref] = dest.file().open_binary_file_for_writing(stringf("%d.png", i));
	verify(stream.get(), "Failed to open PNG file for writing.");
	write_png(*stream, texture);
	
	dest.set_name(stringf("%d", i));
	
	TextureAsset& diffuse = dest.diffuse();
	diffuse.set_src(ref);
}

void unpack_shrub_billboard_texture(
	TextureAsset& dest,
	const ShrubBillboardInfo& billboard,
	InputStream& gs_ram,
	Game game)
{
	std::vector<u8> pixels = gs_ram.read_multiple<u8>(billboard.texture_offset * 0x100, billboard.texture_width * billboard.texture_height);
	std::vector<u32> palette = gs_ram.read_multiple<u32>(billboard.palette_offset * 0x100, 256);
	
	Texture texture = Texture::create_8bit_paletted(billboard.texture_width, billboard.texture_height, pixels, palette);
	texture.multiply_alphas();
	texture.swizzle_palette();
	if (game == Game::DL) {
		texture.swizzle();
	}
	
	auto [stream, ref] = dest.file().open_binary_file_for_writing("billboard.png");
	verify(stream.get(), "Failed to open PNG file for writing.");
	write_png(*stream, texture);
	dest.set_src(ref);
}

SharedLevelTextures read_level_textures(
	const CollectionAsset& tfrag_materials,
	const CollectionAsset& mobies,
	const CollectionAsset& ties,
	const CollectionAsset& shrubs)
{
	SharedLevelTextures shared;
	
	shared.tfrag_range.table = TFRAG_TEXTURE_TABLE;
	shared.tfrag_range.begin = shared.textures.size();
	{
		MaterialSet material_set = read_material_assets(tfrag_materials);
		s32 i = 0;
		for (; i < (s32) material_set.textures.size(); i++) {
			FileReference& texture = material_set.textures[i];
			auto stream = texture.open_binary_file_for_reading();
			shared.textures.emplace_back(LevelTexture{read_png(*stream)});
		}
	}
	shared.tfrag_range.end = shared.textures.size();
	
	shared.moby_range.table = MOBY_TEXTURE_TABLE;
	shared.moby_range.begin = shared.textures.size();
	mobies.for_each_logical_child_of_type<MobyClassAsset>([&](const MobyClassAsset& cls) {
		const CollectionAsset& materials = cls.get_materials();
		MaterialSet material_set = read_material_assets(materials);
		verify(material_set.textures.size() <= 15,
			"Too many textures on moby class '%s'!",
			cls.tag().c_str());
		s32 i = 0;
		for (; i < (s32) material_set.textures.size(); i++) {
			FileReference& texture = material_set.textures[i];
			auto stream = texture.open_binary_file_for_reading();
			shared.textures.emplace_back(LevelTexture{read_png(*stream)});
		}
		for (; i < 16; i++) {
			shared.textures.emplace_back();
		}
	});
	shared.moby_range.end = shared.textures.size();
	
	shared.tie_range.table = TIE_TEXTURE_TABLE;
	shared.tie_range.begin = shared.textures.size();
	ties.for_each_logical_child_of_type<TieClassAsset>([&](const TieClassAsset& cls) {
		const CollectionAsset& materials = cls.get_materials();
		MaterialSet material_set = read_material_assets(materials);
		verify(material_set.textures.size() <= 15,
			"Too many textures on tie class '%s'!",
			cls.tag().c_str());
		s32 i = 0;
		for (; i < (s32) material_set.textures.size(); i++) {
			FileReference& texture = material_set.textures[i];
			auto stream = texture.open_binary_file_for_reading();
			shared.textures.emplace_back(LevelTexture{read_png(*stream)});
		}
		for (; i < 16; i++) {
			shared.textures.emplace_back();
		}
	});
	shared.tie_range.end = shared.textures.size();
	
	shared.shrub_range.table = SHRUB_TEXTURE_TABLE;
	shared.shrub_range.begin = shared.textures.size();
	shrubs.for_each_logical_child_of_type<ShrubClassAsset>([&](const ShrubClassAsset& cls) {
		const CollectionAsset& materials = cls.get_materials();
		MaterialSet material_set = read_material_assets(materials);
		verify(material_set.textures.size() <= 15,
			"Too many textures on shrub class '%s'!",
			cls.tag().c_str());
		s32 i = 0;
		for (; i < (s32) material_set.textures.size(); i++) {
			FileReference& texture = material_set.textures[i];
			auto stream = texture.open_binary_file_for_reading();
			shared.textures.emplace_back(LevelTexture{read_png(*stream)});
		}
		for (; i < 16; i++) {
			shared.textures.emplace_back();
		}
	});
	shared.shrub_range.end = shared.textures.size();
	
	return shared;
}

std::tuple<s32, s32> write_shared_level_textures(
	OutputStream& data,
	OutputStream& gs,
	std::vector<GsRamEntry>& gs_table,
	std::vector<LevelTexture>& textures)
{
	data.pad(0x100, 0);
	s64 base_ofs = data.tell();
	std::vector<u8> mipmap_data;
	
	// Write out regular textures and palettes.
	for (LevelTexture& record : textures) {
		if (record.texture.has_value() && record.out_edge == -1) {
			const Texture& texture = *record.texture;
			
			if (record.palette_out_edge == -1) {
				gs.pad(0x100, 0);
				record.palette_offset = gs.tell();
				gs.write_v(texture.palette());
				gs_table.push_back({PSM_RGBA32, 0, 0, record.palette_offset, record.palette_offset});
			}
			
			if (!record.stashed) {
				mipmap_data.resize((texture.width * texture.height) / 16);
				for (s32 y = 0; y < texture.height / 4; y++) {
					for (s32 x = 0; x < texture.width / 4; x++) {
						mipmap_data[y * (texture.width / 4) + x] = texture.data[y * 4 * texture.width + x * 4];
					}
				}
				
				gs.pad(0x100, 0);
				record.mipmap_offset = gs.tell();
				gs.write_v(mipmap_data);
				
				GsRamEntry mipmap_entry;
				mipmap_entry.psm = PSM_IDTEX8;
				mipmap_entry.width = texture.width / 4;
				mipmap_entry.height = texture.height / 4;
				mipmap_entry.address = record.mipmap_offset;
				mipmap_entry.offset = record.mipmap_offset;
				gs_table.push_back(mipmap_entry);
				
				data.pad(0x100, 0);
				record.texture_offset = data.tell() - base_ofs;
				data.write_v(texture.data);
			}
		}
	}
	
	s32 stash_addr = (s32) gs.tell();
	s32 stash_count = 0;
	
	// Write out stashed (GS memory resident) textures.
	for (LevelTexture& record : textures) {
		if (record.texture.has_value() && record.out_edge == -1 && record.stashed) {
			const Texture& texture = *record.texture;
			
			data.pad(0x100, 0);
			record.texture_offset = (s32) gs.tell() - stash_addr;
			gs.write_v(texture.data);
			
			GsRamEntry entry;
			entry.psm = PSM_IDTEX8;
			entry.width = texture.width;
			entry.height = texture.height;
			entry.address = stash_addr + record.texture_offset;
			entry.offset = record.texture_offset;
			gs_table.push_back(entry);
			
			stash_count++;
		}
	}
	
	return {(s32) base_ofs, stash_count};
}

ArrayRange write_level_texture_table(
	OutputStream& dest, std::vector<LevelTexture>& textures, LevelTextureRange range)
{
	dest.pad(0x10, 0);
	s32 table_offset = dest.tell();
	s32 table_count = 0;
	verify_fatal(range.begin <= range.end);
	for (s32 i = range.begin; i < range.end; i++) {
		LevelTexture* record = &textures.at(i);
		if (record->out_edge > -1) {
			record = &textures.at(record->out_edge);
		}
		// If there already exists an entry in the relevant table for the
		// texture, don't write another one.
		verify_fatal(range.table < 4);
		if (record->texture.has_value() && !record->indices[range.table].has_value()) {
			const Texture& texture = *record->texture;
			verify_fatal(record->texture_offset != -1);
			TextureEntry entry;
			entry.data_offset = record->texture_offset;
			entry.width = texture.width;
			entry.height = texture.height;
			if (record->stashed) {
				entry.type = 0;
			} else {
				entry.type = 3;
			}
			LevelTexture* palette_record = record;
			if (palette_record->palette_out_edge > -1) {
				palette_record = &textures.at(palette_record->palette_out_edge);
			}
			verify_fatal(palette_record->palette_offset != -1);
			entry.palette = palette_record->palette_offset / 0x100;
			if (!record->stashed) {
				entry.mipmap = record->mipmap_offset / 0x100;
			}
			record->indices[range.table] = table_count;
			dest.write(entry);
			table_count++;
		}
	}
	return ArrayRange {table_count, table_offset};
}

void write_level_texture_indices(
	u8 dest[16], const std::vector<LevelTexture>& textures, s32 begin, s32 table)
{
	for (s32 i = 0; i < 16; i++) {
		const LevelTexture* record = &textures.at(begin + i);
		if (record->texture.has_value()) {
			if (record->out_edge > -1) {
				record = &textures[record->out_edge];
			}
			verify_fatal(record->indices[table].has_value());
			verify(*record->indices[table] < 0xff, "Too many textures.\n");
			dest[i] = *record->indices[table];
		} else {
			for (s32 j = i; j < 16; j++) {
				dest[j] = 0xff;
				break;
			}
		}
	}
}

// *****************************************************************************

packed_struct(PartDefsHeader,
	/* 0x0 */ s32 particle_count;
	/* 0x4 */ s32 unknown_4;
	/* 0x8 */ s32 indices_offset;
	/* 0xc */ s32 indices_size;
)

void unpack_particle_textures(
	CollectionAsset& dest,
	InputStream& defs,
	std::vector<ParticleTextureEntry>& entries,
	InputStream& bank,
	Game game)
{
	auto header = defs.read<PartDefsHeader>(0);
	auto offsets = defs.read_multiple<s32>(0x10, header.particle_count);
	auto indices = defs.read_multiple<u8>(header.indices_offset, header.indices_size);
	for (s32 part = 0; part < (s32) offsets.size(); part++) {
		if (offsets[part] == 0) {
			continue;
		}
		
		s32 begin = offsets[part] - header.indices_offset;
		s32 end = header.indices_size;
		for (s32 j = part + 1; j < (s32) offsets.size(); j++) {
			if (offsets[j] != 0) {
				end = offsets[j] - header.indices_offset;
				break;
			}
		}
		verify_fatal(begin >= 0 && end >= begin);
		std::string path = stringf("particle_textures/%d/particle%d.asset", part, part);
		CollectionAsset& part_asset = dest.foreign_child<CollectionAsset>(path, false, part);
		for (s32 frame = begin; frame < end; frame++) {
			u8 index = indices[frame];
			ParticleTextureEntry entry = entries.at(index);
			std::vector<u8> data = bank.read_multiple<u8>(entry.texture, entry.side * entry.side);
			std::vector<u32> palette = bank.read_multiple<u32>(entry.palette, 256);
			Texture texture = Texture::create_8bit_paletted(entry.side, entry.side, data, palette);
			
			if (game == Game::DL) {
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

std::tuple<ArrayRange, std::vector<u8>, s32> pack_particle_textures(
	OutputStream& index, OutputStream& data, const CollectionAsset& particles, Game game)
{
	data.pad(0x100, 0);
	s64 particles_base = data.tell();
	
	s32 particle_count = 0;
	
	std::vector<LevelTexture> textures;
	std::map<s32, std::pair<s32, s32>> ranges;
	for (s32 i = 0; i < 1024; i++) {
		if (particles.has_child(i)) {
			const CollectionAsset& particle = particles.get_child(i).as<CollectionAsset>();
			s32 begin = (s32) textures.size();
			for (s32 j = 0; j < 1024; j++) {
				if (particle.has_child(j)) {
					const TextureAsset& asset = particle.get_child(j).as<TextureAsset>();
					auto stream = asset.src().open_binary_file_for_reading();
					Opt<Texture> texture = read_png(*stream);
					if (texture.has_value()) {
						if (game == Game::DL) {
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
	for (LevelTexture& record : textures) {
		if (record.out_edge > -1) {
			continue;
		}
		
		LevelTexture* palette_record = &record;
		if (palette_record->palette_out_edge > -1) {
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
	
	switch (game) {
		case Game::RAC: particle_count = 0x51; break;
		case Game::GC: particle_count = 0x6f; break;
		case Game::UYA: particle_count = 0x81; break;
		case Game::DL: particle_count = 0x81; break;
		default: {}
	}
	
	std::vector<u8> defs;
	OutBuffer defs_buffer(defs);
	
	// Write out the particle defs.
	defs_buffer.alloc<PartDefsHeader>();
	PartDefsHeader defs_header = {0};
	defs_header.particle_count = particle_count;
	defs_header.indices_size = (s32) textures.size();
	
	s64 offsets_base = defs_buffer.alloc_multiple<s32>(particle_count);
	defs_buffer.pad(0x10, 0);
	defs_header.indices_offset = (s32) defs_buffer.tell();
	
	for (auto [particle, range] : ranges) {
		defs_buffer.write<s32>(offsets_base + particle * 4, defs_header.indices_offset + range.first);
		for (s32 i = range.first; i < range.second; i++) {
			LevelTexture* texture = &textures[i];
			if (texture->out_edge > -1) {
				texture = &textures[texture->out_edge];
			}
			
			verify_fatal(texture->indices[0].has_value());
			defs_buffer.write<u8>(*texture->indices[0]);
		}
	}
	
	defs_buffer.write(0, defs_header);
	
	return {range, defs, particles_base};
}

void unpack_fx_textures(
	LevelWadAsset& core, const std::vector<FxTextureEntry>& entries, InputStream& fx_bank, Game game)
{
	CollectionAsset& fx_textures = core.fx_textures("fx_textures/fx_textures.asset");
	
	std::vector<Texture> textures;
	for (size_t i = 0; i < entries.size(); i++) {
		const FxTextureEntry& entry = entries[i];
		std::vector<u32> palette = fx_bank.read_multiple<u32>(entry.palette, 256);
		std::vector<u8> pixels = fx_bank.read_multiple<u8>(entry.texture, entry.width * entry.height);
		Texture texture = Texture::create_8bit_paletted(entry.width, entry.height, pixels, palette);
	
		if (game == Game::DL) {
			texture.swizzle();
		}
		texture.swizzle_palette();
		texture.multiply_alphas();
		
		std::string name;
		switch (game) {
			case Game::GC: {
				if (i < ARRAY_SIZE(GC_FX_TEXTURE_NAMES)) {
					name = GC_FX_TEXTURE_NAMES[i];
				}
				break;
			}
			case Game::UYA: {
				if (i < ARRAY_SIZE(UYA_FX_TEXTURE_NAMES)) {
					name = UYA_FX_TEXTURE_NAMES[i];
				}
				break;
			}
			case Game::DL: {
				if (i < ARRAY_SIZE(DL_FX_TEXTURE_NAMES)) {
					name = DL_FX_TEXTURE_NAMES[i];
				}
				break;
			}
			default: {}
		}
		
		if (name.empty()) {
			name = std::to_string(i);
		}
		
		TextureAsset& asset = fx_textures.child<TextureAsset>(i);
		auto [stream, ref] = asset.file().open_binary_file_for_writing(name + ".png");
		write_png(*stream, texture);
		asset.set_src(ref);
	}
}

std::tuple<ArrayRange, s32> pack_fx_textures(
	OutputStream& index, OutputStream& data, const CollectionAsset& collection, Game game)
{
	data.pad(0x100, 0);
	s64 fx_base = data.tell();
	
	std::vector<LevelTexture> textures;
	for (s32 i = 0; i < 1024; i++) {
		const TextureAsset* asset;
		if (collection.has_child(i)) {
			asset = &collection.get_child(i).as<TextureAsset>();
		} else {
			break;
		}
		
		auto stream = asset->src().open_binary_file_for_reading();
		Opt<Texture> texture = read_png(*stream);
		if (texture.has_value()) {
			if (game == Game::DL) {
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
	index.pad(0x10, 0);
	range.offset = index.tell();
	for (LevelTexture& texture : textures) {
		LevelTexture* data_texture = &texture;
		if (data_texture->out_edge > -1) {
			data_texture = &textures[data_texture->out_edge];
		}
		
		LevelTexture* palette_texture = data_texture;
		if (palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		FxTextureEntry entry = {-1, -1, -1, -1};
		if (data_texture->texture) {
			entry.palette = palette_texture->palette_offset - fx_base;
			entry.texture = data_texture->texture_offset - fx_base;
			entry.width = data_texture->texture->width;
			entry.height = data_texture->texture->height;
		}
		index.write(entry);
	}
	
	return {range, fx_base};
}

static void write_nonshared_texture_data(
	OutputStream& data, std::vector<LevelTexture>& textures, Game game)
{
	deduplicate_level_textures(textures);
	deduplicate_level_palettes(textures);
	
	for (LevelTexture& record : textures) {
		if (record.out_edge == -1) {
			data.pad(0x100, 0);
			if (record.palette_out_edge == -1) {
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

void deduplicate_level_textures(std::vector<LevelTexture>& textures)
{
	std::vector<size_t> mapping;
	for (size_t i = 0; i < textures.size(); i++) {
		if (textures[i].texture.has_value()) {
			mapping.push_back(i);
		}
	}
	
	if (mapping.size() < 1) {
		return;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return *textures[lhs].texture < *textures[rhs].texture;
	});
	
	std::vector<size_t> group{mapping[0]};
	auto merge_group = [&]() {
		size_t lowest = SIZE_MAX;
		for (size_t index : group) {
			lowest = std::min(lowest, index);
		}
		verify_fatal(lowest != SIZE_MAX);
		bool stashed = false;
		for (size_t index : group) {
			stashed |= textures[index].stashed;
		}
		for (size_t index : group) {
			if (index != lowest) {
				textures[index].stashed = stashed;
				textures[index].out_edge = lowest;
			}
		}
	};
	
	for (size_t i = 1; i < mapping.size(); i++) {
		LevelTexture& last = textures[mapping[i - 1]];
		LevelTexture& cur = textures[mapping[i]];
		if (!(*last.texture == *cur.texture)) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if (group.size() > 0) {
		merge_group();
	}
}

void deduplicate_level_palettes(std::vector<LevelTexture>& textures)
{
	std::vector<size_t> mapping;
	for (size_t i = 0; i < textures.size(); i++) {
		if (textures[i].texture.has_value() && textures[i].out_edge == -1) {
			mapping.push_back(i);
		}
	}
	
	if (mapping.size() < 1) {
		return;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return textures[lhs].texture->palette() < textures[rhs].texture->palette();
	});
	
	std::vector<size_t> group{mapping[0]};
	auto merge_group = [&]() {
		size_t lowest = SIZE_MAX;
		for (size_t index : group) {
			lowest = std::min(lowest, index);
		}
		verify_fatal(lowest != SIZE_MAX);
		for (size_t index : group) {
			if (index != lowest) {
				textures[index].palette_out_edge = lowest;
			}
		}
	};
	
	for (size_t i = 1; i < mapping.size(); i++) {
		LevelTexture& last = textures[mapping[i - 1]];
		LevelTexture& cur = textures[mapping[i]];
		if (!(last.texture->palette() == cur.texture->palette())) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if (group.size() > 0) {
		merge_group();
	}
}

const char* GC_FX_TEXTURE_NAMES[63] = {
	/* 00 */ "lame_shadow",
	/* 01 */ "font_1",
	/* 02 */ "font_2",
	/* 03 */ "font_3",
	/* 04 */ "gadgetron",
	/* 05 */ "engine",
	/* 06 */ "6",
	/* 07 */ "7",
	/* 08 */ "8",
	/* 09 */ "ship_shadow",
	/* 10 */ "jp_thrust_fire",
	/* 11 */ "jp_thrust_glow",
	/* 12 */ "jp_thrust_highlight",
	/* 13 */ "target_reticule",
	/* 14 */ "lightning1",
	/* 15 */ "15",
	/* 16 */ "glow_pill",
	/* 17 */ "17",
	/* 18 */ "18",
	/* 19 */ "sparkle",
	/* 20 */ "wrench_blur",
	/* 21 */ "suck_tornado",
	/* 22 */ "white",
	/* 23 */ "alpha_spark",
	/* 24 */ "24",
	/* 25 */ "tv_highlight",
	/* 26 */ "tv_smallscan",
	/* 27 */ "halo",
	/* 28 */ "tv_scanlines",
	/* 29 */ "tv_shine",
	/* 30 */ "tv_noise",
	/* 31 */ "31",
	/* 32 */ "32",
	/* 33 */ "33",
	/* 34 */ "34",
	/* 35 */ "ryno_reticule",
	/* 36 */ "swingshot_reticule",
	/* 37 */ "static",
	/* 38 */ "blaster_reticule",
	/* 39 */ "devastator_reticule",
	/* 40 */ "40",
	/* 41 */ "plasma_ball_core",
	/* 42 */ "plasma_ball_aura",
	/* 43 */ "plasma_lightning_bolt",
	/* 44 */ "44",
	/* 45 */ "plasma_ball_glow_ring",
	/* 46 */ "steam_smoke_gas",
	/* 47 */ "fork_lightning",
	/* 48 */ "fork_lightning_glow_core",
	/* 49 */ "starry_flash",
	/* 50 */ "lava_glob",
	/* 51 */ "main_ret1",
	/* 52 */ "main_ret2",
	/* 53 */ "main_ret3",
	/* 54 */ "54",
	/* 55 */ "55",
	/* 56 */ "shockwave",
	/* 57 */ "explosion",
	/* 58 */ "radialblur_sniper",
	/* 59 */ "59",
	/* 60 */ "60",
	/* 61 */ "61",
	/* 62 */ "62"
};

const char* UYA_FX_TEXTURE_NAMES[100] = {
	/* 00 */ "lame_shadow",
	/* 01 */ "01",
	/* 02 */ "02",
	/* 03 */ "03",
	/* 04 */ "gadgetron",
	/* 05 */ "engine",
	/* 06 */ "06",
	/* 07 */ "07",
	/* 08 */ "08",
	/* 09 */ "ship_shadow",
	/* 10 */ "jp_thrust_fire",
	/* 11 */ "jp_thrust_glow",
	/* 12 */ "jp_thrust_highlight",
	/* 13 */ "target_reticule",
	/* 14 */ "lightning1",
	/* 15 */ "15",
	/* 16 */ "glow_pill",
	/* 17 */ "17",
	/* 18 */ "18",
	/* 19 */ "sparkle",
	/* 20 */ "wrench_blur",
	/* 21 */ "suck_tornado",
	/* 22 */ "white",
	/* 23 */ "alpha_spark",
	/* 24 */ "hologram",
	/* 25 */ "tv_highlight",
	/* 26 */ "tv_smallscan",
	/* 27 */ "halo",
	/* 28 */ "tv_scanlines",
	/* 29 */ "tv_shine",
	/* 30 */ "tv_noise",
	/* 31 */ "triangle_reticule",
	/* 32 */ "32",
	/* 33 */ "33",
	/* 34 */ "34",
	/* 35 */ "ryno_reticule",
	/* 36 */ "swingshot_reticule",
	/* 37 */ "static",
	/* 38 */ "blaster_reticule",
	/* 39 */ "devastator_reticule",
	/* 40 */ "40",
	/* 41 */ "plasma_ball_core",
	/* 42 */ "plasma_ball_aura",
	/* 43 */ "43",
	/* 44 */ "plasma_ball_flare",
	/* 45 */ "plasma_ball_glow_ring",
	/* 46 */ "steam_smoke_gas",
	/* 47 */ "fork_lightning",
	/* 48 */ "fork_lightning_glow_core",
	/* 49 */ "starry_flash",
	/* 50 */ "lava_glob",
	/* 51 */ "main_ret1",
	/* 52 */ "main_ret2",
	/* 53 */ "main_ret3",
	/* 54 */ "smoke_ring",
	/* 55 */ "explotype1",
	/* 56 */ "shockwave",
	/* 57 */ "explosion",
	/* 58 */ "plasma_shot",
	/* 59 */ "heatmask2",
	/* 60 */ "60",
	/* 61 */ "shockwave01_keith",
	/* 62 */ "muzzleflash1",
	/* 63 */ "muzzleflash2",
	/* 64 */ "streamer_keith",
	/* 65 */ "muzzle_flower",
	/* 66 */ "radialblur_sniper",
	/* 67 */ "holoshield_base",
	/* 68 */ "68",
	/* 69 */ "refractor_beam",
	/* 70 */ "70",
	/* 71 */ "starburst1_keith",
	/* 72 */ "starburst2_keith",
	/* 73 */ "firecircle02_keith",
	/* 74 */ "halfring_keith",
	/* 75 */ "whirlpool_keith",
	/* 76 */ "corona_keith",
	/* 77 */ "pinch_alpha_mask",
	/* 78 */ "78",
	/* 79 */ "duck_feather2",
	/* 80 */ "80",
	/* 81 */ "81",
	/* 82 */ "82",
	/* 83 */ "83",
	/* 84 */ "84",
	/* 85 */ "warpout_shockwave",
	/* 86 */ "n60_reticule",
	/* 87 */ "87",
	/* 88 */ "ground2_reticule",
	/* 89 */ "health_ball",
	/* 90 */ "discblade_reticule",
	/* 91 */ "shockblaster_reticule",
	/* 92 */ "character_al",
	/* 93 */ "character_helfa",
	/* 94 */ "character_qwark",
	/* 95 */ "character_skrunch",
	/* 96 */ "character_skidd",
	/* 97 */ "character_slim",
	/* 98 */ "character_sasha",
	/* 99 */ "character_president"
};

const char* DL_FX_TEXTURE_NAMES[98] = {
	/* 00 */ "lame_shadow",
	/* 01 */ "ground_outer_reticule",
	/* 02 */ "ground_inner_reticule",
	/* 03 */ "center_screen_reticule1",
	/* 04 */ "center_screen_reticule2",
	/* 05 */ "generic_reticule",
	/* 06 */ "cmd_attack",
	/* 07 */ "cmd_defend",
	/* 08 */ "cmd_emp",
	/* 09 */ "cmd_shield",
	/* 10 */ "cmd_mine",
	/* 11 */ "jp_thrust_glow",
	/* 12 */ "jp_thrust_highlight",
	/* 13 */ "jp_thrust_fire",
	/* 14 */ "lightning1",
	/* 15 */ "engine",
	/* 16 */ "glow_pill",
	/* 17 */ "lens_flare_2",
	/* 18 */ "ship_shadow",
	/* 19 */ "sparkle",
	/* 20 */ "wrench_blur",
	/* 21 */ "suck_tornado",
	/* 22 */ "white",
	/* 23 */ "alpha_spark",
	/* 24 */ "hologram",
	/* 25 */ "tv_highlight",
	/* 26 */ "tv_smallscan",
	/* 27 */ "halo",
	/* 28 */ "tv_scanlines",
	/* 29 */ "tv_shine",
	/* 30 */ "target_reticule",
	/* 31 */ "cone_fire01_slim",
	/* 32 */ "sandstorm",
	/* 33 */ "progressbar_inner",
	/* 34 */ "progressbar_outer",
	/* 35 */ "ryno_reticule",
	/* 36 */ "swingshot_reticule",
	/* 37 */ "static",
	/* 38 */ "blaster_reticule",
	/* 39 */ "devastator_reticule",
	/* 40 */ "triangle_reticule",
	/* 41 */ "plasma_ball_core",
	/* 42 */ "plasma_ball_aura",
	/* 43 */ "plasma_lightning_bolt",
	/* 44 */ "plasma_ball_flare",
	/* 45 */ "plasma_ball_glow_ring",
	/* 46 */ "steam_smoke_gas",
	/* 47 */ "fork_lightning",
	/* 48 */ "fork_lightning_glow_core",
	/* 49 */ "starry_flash",
	/* 50 */ "lava_glob",
	/* 51 */ "main_ret1",
	/* 52 */ "main_ret2",
	/* 53 */ "main_ret3",
	/* 54 */ "smoke_ring",
	/* 55 */ "explotype1",
	/* 56 */ "shockwave",
	/* 57 */ "explosion",
	/* 58 */ "plasma_shot",
	/* 59 */ "heatmask2",
	/* 60 */ "concrete",
	/* 61 */ "shockwave01_keith",
	/* 62 */ "muzzleflash1",
	/* 63 */ "muzzleflash2",
	/* 64 */ "streamer_keith",
	/* 65 */ "muzzle_flower",
	/* 66 */ "radialblur_sniper",
	/* 67 */ "holoshield_base",
	/* 68 */ "sniper_outer_reticule",
	/* 69 */ "refractor_beam",
	/* 70 */ "sniper_inner_reticule",
	/* 71 */ "starburst1_keith",
	/* 72 */ "starburst2_keith",
	/* 73 */ "firecircle02_keith",
	/* 74 */ "halfring_keith",
	/* 75 */ "whirlpool_keith",
	/* 76 */ "corona_keith",
	/* 77 */ "pinch_alpha_mask",
	/* 78 */ "duck_feather1",
	/* 79 */ "duck_feather2",
	/* 80 */ "cell_stream01",
	/* 81 */ "cell_stream02",
	/* 82 */ "bullet_trail_slim",
	/* 83 */ "lightning02_keith",
	/* 84 */ "lightning01_slim",
	/* 85 */ "warpout_shockwave",
	/* 86 */ "n60_reticule",
	/* 87 */ "ground1_reticule",
	/* 88 */ "ground2_reticule",
	/* 89 */ "health_ball",
	/* 90 */ "discblade_reticule",
	/* 91 */ "shockblaster_reticule",
	/* 92 */ "focus_ratchet_red",
	/* 93 */ "focus_ratchet_blue",
	/* 94 */ "focus_ratchet_red_dead",
	/* 95 */ "focus_ratchet_blue_dead",
	/* 96 */ "lock_on_reticule",
	/* 97 */ "cracks",
};
