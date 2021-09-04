/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

std::vector<PalettedTexture> write_nonshared_textures(OutBuffer data, const std::vector<Texture>& src);
static Texture paletted_texture_to_full_colour(Buffer data, Buffer palette, s32 width, s32 height);
static u8 decode_palette_index(u8 index);
static std::optional<std::array<u8, 256>> attempt_merge_palettes(Palette& subset, Palette& superset);

std::vector<Texture> read_tfrag_textures(BufferArray<TextureEntry> texture_table, Buffer data, Buffer gs_ram) {
	std::vector<Texture> textures;
	for(const TextureEntry& entry : texture_table) {
		Buffer texture = data.subbuf(entry.data_offset);
		Buffer palette = gs_ram.subbuf(entry.palette * 0x100);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.width, entry.height));
	}
	return textures;
}

std::vector<Texture> read_instance_textures(BufferArray<TextureEntry> texture_table, const u8 indices[16], Buffer data, Buffer gs_ram) {
	std::vector<Texture> textures;
	for(s32 i = 0; i < 16; i++) {
		if(indices[i] == 0xff) {
			break;
		}
		const TextureEntry& entry = texture_table[indices[i]];
		Buffer texture = data.subbuf(entry.data_offset);
		Buffer palette = gs_ram.subbuf(entry.palette * 0x100);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.width, entry.height));
	}
	return textures;
}

std::vector<Texture> read_particle_textures(BufferArray<ParticleTextureEntry> texture_table, Buffer data) {
	std::vector<Texture> textures;
	for(const ParticleTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.side, entry.side));
	}
	return textures;
}

ArrayRange write_particle_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& src) {
	s64 particle_base = data.tell();
	std::vector<PalettedTexture> textures = write_nonshared_textures(data, src);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(PalettedTexture& texture : textures) {
		PalettedTexture* palette_texture = &texture;
		while(palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		ParticleTextureEntry entry;
		entry.palette = palette_texture->palette_offset - particle_base;
		entry.unknown_4 = 0;
		entry.texture = texture.texture_offset - particle_base;
		entry.side = texture.width;
		header.write(entry);
	}
	return range;
}

std::vector<Texture> read_fx_textures(BufferArray<FXTextureEntry> texture_table, Buffer data) {
	std::vector<Texture> textures;
	for(const FXTextureEntry& entry : texture_table) {
		Buffer palette = data.subbuf(entry.palette);
		Buffer texture = data.subbuf(entry.texture);
		textures.emplace_back(paletted_texture_to_full_colour(texture, palette, entry.width, entry.height));
	}
	return textures;
}

ArrayRange write_fx_textures(OutBuffer header, OutBuffer data, const std::vector<Texture>& src) {
	s64 fx_base = data.tell();
	std::vector<PalettedTexture> textures = write_nonshared_textures(data, src);
	ArrayRange range;
	range.count = textures.size();
	range.offset = header.tell();
	for(PalettedTexture& texture : textures) {
		PalettedTexture* tex = &texture;
		PalettedTexture* palette_texture = &texture;
		while(palette_texture->palette_out_edge > -1) {
			palette_texture = &textures[palette_texture->palette_out_edge];
		}
		
		FXTextureEntry entry;
		entry.palette = palette_texture->palette_offset - fx_base;
		entry.texture = tex->texture_offset - fx_base;
		entry.width = texture.width;
		entry.height = texture.height;
		header.write(entry);
	}
	return range;
}

std::vector<PalettedTexture> write_nonshared_textures(OutBuffer data, const std::vector<Texture>& src) {
	std::vector<PalettedTexture> textures;
	textures.reserve(src.size());
	for(const Texture& texture : src) {
		textures.emplace_back(find_suboptimal_palette(texture));
	}
	deduplicate_palettes(textures);
	for(PalettedTexture& texture : textures) {
		assert(texture.texture_out_edge == -1 || textures[texture.texture_out_edge].texture_out_edge == -1);
	}
	encode_palette_indices(textures);
	
	for(PalettedTexture& texture : textures) {
		if(texture.palette_out_edge == -1) {
			texture.palette_offset = data.write_multiple(texture.palette.colours);
		}
		texture.texture_offset = data.write_multiple(texture.data);
	}
	return textures;
}

static Texture paletted_texture_to_full_colour(Buffer data, Buffer palette, s32 width, s32 height) {
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.data.resize(width * height);
	for(s32 i = 0; i < width * height; i++) {
		texture.data[i] = palette.read<u32>(decode_palette_index(data.read<u8>(i, "texture")) * 4, "palette");
		u32 alpha = (texture.data[i] & 0xff000000) >> 24;
		alpha = std::min(alpha * 2, 0xffu);
		texture.data[i] = (texture.data[i] & 0x00ffffff) | (alpha << 24);
	}
	return texture;
}

std::pair<std::vector<const Texture*>, FlattenedTextureLayout> flatten_textures(const LevelWad& wad) {
	std::vector<const Texture*> pointers;
	pointers.reserve(wad.tfrag_textures.size());
	FlattenedTextureLayout layout;
	layout.tfrags_begin = pointers.size();
	for(const Texture& texture : wad.tfrag_textures) {
		pointers.push_back(&texture);
	}
	layout.mobies_begin = pointers.size();
	for(const MobyClass& cls : wad.moby_classes) {
		for(const Texture& texture : cls.textures) {
			pointers.push_back(&texture);
		}
	}
	layout.ties_begin = pointers.size();
	for(const TieClass& cls : wad.tie_classes) {
		for(const Texture& texture : cls.textures) {
			pointers.push_back(&texture);
		}
	}
	layout.shrubs_begin = pointers.size();
	for(const ShrubClass& cls : wad.shrub_classes) {
		for(const Texture& texture : cls.textures) {
			pointers.push_back(&texture);
		}
	}
	return {pointers, layout};
}

PalettedTexture find_suboptimal_palette(const Texture& src) {
	assert(src.data.size() == src.width * src.height);
	
	PalettedTexture texture = {0};
	texture.width = src.width;
	texture.height = src.height;
	texture.data.resize(texture.width * texture.height);
	texture.path = src.path;
	for(s32 i = 0; i < texture.width * texture.height; i++) {
		s32 match = -1;
		for(s32 j = 0; j < texture.palette.top; j++) {
			if(texture.palette.colours[j] == src.data[i]) {
				match = j;
				break;
			}
		}
		if(match >= 0) {
			texture.data[i] = (u8) match;
		} else if(texture.palette.top < 256) {
			texture.palette.colours[texture.palette.top] = src.data[i];
			texture.data[i] = texture.palette.top;
			texture.palette.top++;
		} else {
			texture.data[i] = 0;
		}
	}
	
	return texture;
}

void deduplicate_textures(std::vector<PalettedTexture>& textures) {
	std::vector<size_t> mapping(textures.size());
	for(size_t i = 0; i < textures.size(); i++) {
		mapping[i] = i;
	}
	
	std::sort(BEGIN_END(mapping), [&](size_t lhs, size_t rhs) {
		return textures[lhs].data < textures[rhs].data;
	});
	
	s32 first_occurence = mapping[0];
	textures[mapping[0]].is_first_occurence = true;
	for(size_t i = 1; i < textures.size(); i++) {
		PalettedTexture& last = textures[mapping[i - 1]];
		PalettedTexture& cur = textures[mapping[i]];
		// Maybe in the future we could do something clever here to find cases
		// where the pixel data is duplicated but the palette isn't. That may
		// save EE memory in some cases, but may complicate mipmap generation.
		if(last.data == cur.data && last.palette == cur.palette) {
			cur.texture_out_edge = first_occurence;
			cur.is_first_occurence = false;
		} else {
			first_occurence = mapping[i];
			cur.is_first_occurence = true;
		}
	}
}

void deduplicate_palettes(std::vector<PalettedTexture>& textures) {
	for(size_t subset = 0; subset < textures.size(); subset++) {
		PalettedTexture& subtex = textures[subset];
		if(!subtex.is_first_occurence) {
			continue;
		}
		for(size_t superset = 0; superset < textures.size(); superset++) {
			if(subset == superset) {
				continue;
			}
			PalettedTexture& supertex = textures[superset];
			if(!supertex.is_first_occurence || subtex.palette_out_edge != -1) {
				continue;
			}
			// If a palette A has more colours than a palette B, then A cannot
			// contain a subset of the colours in B.
			if(subtex.palette.top > supertex.palette.top) {
				continue;
			}
			// If two palettes have the same number of colours, the only way one
			// can be a subset of another is if they are permutations of each
			// other, hence they we should discard half of these subsets to
			// avoid cycles.
			if(subtex.palette.top >= supertex.palette.top && subset < superset) {
				continue;
			}
			auto mapping = attempt_merge_palettes(subtex.palette, textures[superset].palette);
			if(mapping.has_value()) {
				subtex.palette_out_edge = superset;
				for(u8& pixel : subtex.data) {
					pixel = (*mapping)[pixel];
				}
			}
		}
	}
}

static std::optional<std::array<u8, 256>> attempt_merge_palettes(Palette& subset, Palette& superset) {
	std::array<u8, 256> mapping = {0};
	static std::vector<u32> missing_colours(256);
	missing_colours.clear();
	for(s32 i = 0; i < subset.top; i++) {
		bool found_colour = false;
		for(s32 j = 0; j < 256; j++) {
			if(subset.colours[i] == superset.colours[j]) {
				found_colour = true;
				mapping[i] = j;
				break;
			}
		}
		if(!found_colour) {
			mapping[i] = superset.top + missing_colours.size();
			missing_colours.push_back(subset.colours[i]);
		}
	}
	if(missing_colours.size() < 256 - superset.top) {
		memcpy(&superset.colours[superset.top], missing_colours.data(), missing_colours.size() * 4);
		superset.top += missing_colours.size();
		return mapping;
	} else {
		return {};
	}
}

void encode_palette_indices(std::vector<PalettedTexture>& textures) {
	for(PalettedTexture& texture : textures) {
		if(texture.is_first_occurence) {
			for(u8& pixel : texture.data) {
				pixel = decode_palette_index(pixel);
			}
		}
	}
}

static u8 decode_palette_index(u8 index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}
