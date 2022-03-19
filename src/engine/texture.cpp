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

#include "texture.h"

static std::vector<TextureDedupeRecord> write_nonshared_texture_data(OutBuffer data, const std::vector<Texture>& textures);
static Texture read_paletted_texture(Buffer data, Buffer palette, s32 width, s32 height, Game game);
static u8 map_palette_index(u8 index);
static s32 remap_pixel_index_rac4(s32 i, s32 width);

Texture read_shared_texture(Buffer texture, Buffer palette, TextureEntry entry, Game game) {
	return read_paletted_texture(texture.subbuf(entry.data_offset), palette.subbuf(entry.palette * 0x100), entry.width, entry.height, game);
}

s64 write_shared_texture_data(OutBuffer ee, OutBuffer gs, std::vector<GsRamEntry>& table, std::vector<TextureDedupeRecord>& records) {
	ee.pad(0x40);
	s64 ofs = ee.tell();
	std::vector<u8> mipmap_data;
	for(TextureDedupeRecord& record : records) {
		if(record.texture != nullptr && record.texture_out_edge == -1) {
			const Texture& texture = *record.texture;
			if(record.palette_out_edge == -1) {
				gs.pad(0x100, 0);
				record.palette_offset = write_palette(gs, texture.palette);
				table.push_back({0, 0, 0, record.palette_offset, record.palette_offset});
			}
			mipmap_data.resize((texture.width * texture.height) / 16);
			for(s32 y = 0; y < texture.height / 4; y++) {
				for(s32 x = 0; x < texture.width / 4; x++) {
					mipmap_data[y * (texture.width / 4) + x] = texture.pixels[y * 4 * texture.width + x * 4];
				}
			}
			gs.pad(0x100, 0);
			record.mipmap_offset = gs.write_multiple(mipmap_data);
			GsRamEntry mipmap_entry;
			mipmap_entry.unknown_0 = 0x13;
			mipmap_entry.width = texture.width / 4;
			mipmap_entry.height = texture.height / 4;
			mipmap_entry.offset_1 = record.mipmap_offset;
			mipmap_entry.offset_2 = record.mipmap_offset;
			table.push_back(mipmap_entry);
			ee.pad(0x100, 0);
			record.texture_offset = ee.write_multiple(texture.pixels);
		}
	}
	return ofs;
}

std::vector<Texture> read_particle_textures(BufferArray<ParticleTextureEntry> texture_table, Buffer data, Game game) {
	std::vector<Texture> textures;
	for(const ParticleTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(read_paletted_texture(texture, palette, entry.side, entry.side, game));
	}
	return textures;
}

ArrayRange write_particle_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& textures) {
	s64 particle_base = data.tell();
	auto records = write_nonshared_texture_data(data, textures);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(TextureDedupeRecord& record : records) {
		TextureDedupeRecord* palette_record = &record;
		if(palette_record->palette_out_edge > -1) {
			palette_record = &records[palette_record->palette_out_edge];
		}
		
		ParticleTextureEntry entry;
		entry.palette = palette_record->palette_offset - particle_base;
		entry.unknown_4 = 0;
		entry.texture = record.texture_offset - particle_base;
		entry.side = record.texture->width;
		header.write(entry);
	}
	return range;
}

std::vector<Texture> read_fx_textures(BufferArray<FXTextureEntry> texture_table, Buffer data, Game game) {
	std::vector<Texture> textures;
	for(const FXTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(read_paletted_texture(texture, palette, entry.width, entry.height, game));
	}
	return textures;
}

ArrayRange write_fx_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& textures) {
	s64 fx_base = data.tell();
	auto records = write_nonshared_texture_data(data, textures);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(TextureDedupeRecord& record : records) {
		TextureDedupeRecord* palette_record = &record;
		if(palette_record->palette_out_edge > -1) {
			palette_record = &records[palette_record->palette_out_edge];
		}
		
		FXTextureEntry entry;
		entry.palette = palette_record->palette_offset - fx_base;
		entry.texture = record.texture_offset - fx_base;
		entry.width = record.texture->width;
		entry.height = record.texture->height;
		header.write(entry);
	}
	return range;
}

static std::vector<TextureDedupeRecord> write_nonshared_texture_data(OutBuffer data, const std::vector<Texture>& textures) {
	std::vector<TextureDedupeRecord> records;
	for(const Texture& texture : textures) {
		records.emplace_back(TextureDedupeRecord {&texture});
	}
	
	deduplicate_palettes(records);
	
	for(TextureDedupeRecord& record : records) {
		if(record.palette_out_edge == -1) {
			record.palette_offset = write_palette(data, record.texture->palette);
		}
		record.texture_offset = data.write_multiple(record.texture->pixels);
	}
	
	return records;
}

Texture read_paletted_texture(Buffer data, Buffer palette, s32 width, s32 height, Game game) {
	Texture texture = {0};
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::IDTEX8;
	texture.palette.top = 256;
	for(s32 i = 0; i < 256; i++) {
		texture.palette.colours[i] = palette.read<u32>(map_palette_index(i) * 4, "palette");
		u32 alpha = (texture.palette.colours[i] & 0xff000000) >> 24;
		alpha = std::min(alpha * 2, 0xffu);
		texture.palette.colours[i] = (texture.palette.colours[i] & 0x00ffffff) | (alpha << 24);
	}
	auto pixels = data.read_multiple<u8>(0, width * height, "texture").copy();
	if(game == Game::DL && width >= 32 && height >= 4) {
		s32 buffer_size = width * height;
		texture.pixels.resize(buffer_size);
		for(size_t i = 0; i < buffer_size; i++) {
			s32 map = remap_pixel_index_rac4(i, width);
			if(map >= buffer_size) {
				map = buffer_size - 1;
			}
			texture.pixels[map] = pixels[i];
		}
	} else {
		texture.pixels = std::move(pixels);
	}
	return texture;
}

TextureDedupeOutput prepare_texture_dedupe_records(TextureDedupeInput& input) {
	std::vector<TextureDedupeRecord> records;
	
	s32 tfrags_begin = (s32) records.size();
	for(const Texture& texture : input.tfrag_textures) {
		records.emplace_back(TextureDedupeRecord{&texture});
	}
	
	s32 mobies_begin = (s32) records.size();
	for(const MobyClass& cls : input.moby_classes) {
		verify(cls.textures.size() < 16, "Moby class %d has too many textures.", cls.o_class);
		for(const Texture& texture : cls.textures) {
			records.emplace_back(TextureDedupeRecord{&texture});
		}
		for(size_t i = cls.textures.size(); i < 16; i++) {
			records.emplace_back(TextureDedupeRecord{nullptr});
		}
	}
	
	s32 ties_begin = (s32) records.size();
	for(const TieClass& cls : input.tie_classes) {
		verify(cls.textures.size() < 16, "Tie class %d has too many textures.", cls.o_class);
		for(const Texture& texture : cls.textures) {
			records.emplace_back(TextureDedupeRecord{&texture});
		}
		for(size_t i = cls.textures.size(); i < 16; i++) {
			records.emplace_back(TextureDedupeRecord{nullptr});
		}
	}
	
	s32 shrubs_begin = (s32) records.size();
	for(const ShrubClass& cls : input.shrub_classes) {
		verify(cls.textures.size() < 16, "Shrub class %d has too many textures.", cls.o_class);
		for(const Texture& texture : cls.textures) {
			records.emplace_back(TextureDedupeRecord{&texture});
		}
		for(size_t i = cls.textures.size(); i < 16; i++) {
			records.emplace_back(TextureDedupeRecord{nullptr});
		}
	}
	
	return {records, tfrags_begin, mobies_begin, ties_begin, shrubs_begin};
}

void deduplicate_textures(std::vector<TextureDedupeRecord>& records) {
	std::vector<size_t> mapping;
	for(size_t i = 0; i < records.size(); i++) {
		if(records[i].texture != nullptr) {
			mapping.push_back(i);
		}
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		if(!(*records[lhs].texture == *records[rhs].texture)) return *records[lhs].texture < *records[rhs].texture;
		return records[lhs].texture->palette.colours < records[rhs].texture->palette.colours;
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
				records[index].texture_out_edge = lowest;
			}
		}
	};
	
	for(size_t i = 1; i < mapping.size(); i++) {
		TextureDedupeRecord& last = records[mapping[i - 1]];
		TextureDedupeRecord& cur = records[mapping[i]];
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

void deduplicate_palettes(std::vector<TextureDedupeRecord>& records) {
	std::vector<size_t> mapping;
	for(size_t i = 0; i < records.size(); i++) {
		if(records[i].texture != nullptr && records[i].texture_out_edge == -1) {
			mapping.push_back(i);
		}
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return records[lhs].texture->palette.colours < records[rhs].texture->palette.colours;
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
				records[index].palette_out_edge = lowest;
			}
		}
	};
	
	for(size_t i = 1; i < mapping.size(); i++) {
		TextureDedupeRecord& last = records[mapping[i - 1]];
		TextureDedupeRecord& cur = records[mapping[i]];
		if(!(last.texture->palette == cur.texture->palette)) {
			merge_group();
			group.clear();
		}
		group.push_back(mapping[i]);
	}
	if(group.size() > 0) {
		merge_group();
	}
}

s64 write_palette(OutBuffer dest, const Palette& palette) {
	s64 ofs = dest.tell();
	for(s32 i = 0; i < 256; i++) {
		u32 colour = palette.colours[map_palette_index(i)];
		u32 alpha = (colour & 0xff000000) >> 24;
		if(alpha != 0xff) {
			alpha = alpha / 2;
		} else {
			alpha = 0x80;
		}
		dest.write((colour & 0x00ffffff) | (alpha << 24));
	}
	return ofs;
}

static u8 map_palette_index(u8 index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}

static s32 remap_pixel_index_rac4(s32 i, s32 width) {
	s32 s = i / (width * 2);
	s32 r = 0;
	if (s % 2 == 0)
		r = s * 2;
	else
		r = (s - 1) * 2 + 1;

	s32 q = ((i % (width * 2)) / 32);

	s32 m = i % 4;
	s32 n = (i / 4) % 4;
	s32 o = i % 2;
	s32 p = (i / 16) % 2;

	if ((s / 2) % 2 == 1)
		p = 1 - p;

	if (o == 0)
		m = (m + p) % 4;
	else
		m = ((m - p) + 4) % 4;


	s32 x = n + ((m + q * 4) * 4);
	s32 y = r + (o * 2);

	return (x % width) + (y * width);
}
