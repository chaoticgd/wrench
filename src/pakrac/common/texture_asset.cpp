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

#include <core/png.h>
#include <pakrac/asset_unpacker.h>
#include <pakrac/asset_packer.h>

static void unpack_texture_asset(TextureAsset& dest, InputStream& src, Game game, AssetFormatHint hint);
static void pack_texture_asset(OutputStream& dest, TextureAsset& src, Game game, AssetFormatHint hint);
static Texture unpack_pif(InputStream& src);
static void pack_pif(OutputStream& dest, TextureAsset& src, AssetFormatHint hint);
static bool test_texture_asset(std::vector<u8>& original, std::vector<u8>& repacked, Game game, AssetFormatHint hint);

on_load(Texture, []() {
	TextureAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_dl = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	
	TextureAsset::funcs.pack_rac1 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_rac2 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_rac3 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_dl = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	
	TextureAsset::funcs.test = new AssetTestFunc(test_texture_asset);
})

static void unpack_texture_asset(TextureAsset& dest, InputStream& src, Game game, AssetFormatHint hint) {
	switch(hint) {
		default: {
			Texture texture = unpack_pif(src);
			auto [file, ref] = dest.file().open_binary_file_for_writing(dest.tag() + ".png");
			write_png(*file, texture);
			dest.set_src(ref);
		}
	}
}

static void pack_texture_asset(OutputStream& dest, TextureAsset& src, Game game, AssetFormatHint hint) {
	switch(hint) {
		default: {
			pack_pif(dest, src, hint);
		}
	}
}

packed_struct(PifHeader,
	/* 0x00 */ char magic[4];
	/* 0x04 */ s32 file_size;
	/* 0x08 */ s32 width;
	/* 0x0c */ s32 height;
	/* 0x10 */ s32 format;
	/* 0x14 */ s32 clut_format;
	/* 0x18 */ s32 clut_order;
	/* 0x1c */ s32 mip_levels;
)

static Texture unpack_pif(InputStream& src) {
	PifHeader header = src.read<PifHeader>(0);
	verify(memcmp(header.magic, "2FIP", 4) == 0, "PIF has bad magic bytes.");
	verify(header.width <= 2048 && header.height <= 2048, "PIF has bad width/height values.");
	
	switch(header.format) {
		case 0x13: {
			std::vector<u32> palette(256);
			src.read_n((u8*) palette.data(), palette.size() * 4);
			std::vector<u8> data(header.width * header.height);
			src.read_n(data.data(), data.size());
			Texture texture = Texture::create_8bit_paletted(header.width, header.height, data, palette);
			texture.swizzle_palette();
			texture.multiply_alphas();
			return texture;
		}
		case 0x94: {
			std::vector<u32> palette(16);
			src.read_n((u8*) palette.data(), palette.size() * 4);
			std::vector<u8> data((header.width * header.height) / 2);
			src.read_n(data.data(), data.size());
			Texture texture = Texture::create_4bit_paletted(header.width, header.height, data, palette);
			texture.multiply_alphas();
			return texture;
		}
		default: {
			verify_not_reached("PIF has invalid format field.");
		}
	}

}

static void pack_pif(OutputStream& dest, TextureAsset& src, AssetFormatHint hint) {
	auto stream = src.file().open_binary_file_for_reading(src.src());
	verify(stream.get(), "Failed to open PNG file.");
	Opt<Texture> texture = read_png(*stream);
	verify(texture.has_value(), "Failed to read PNG file.");
	
	texture->divide_alphas();
	
	s64 header_ofs = dest.tell();
	PifHeader header = {0};
	dest.write(header);
	memcpy(header.magic, "2FIP", 4);
	header.width = texture->width;
	header.height = texture->height;
	header.mip_levels = 1;
	
	switch(hint) {
		case FMT_TEXTURE_PIF4:
		case FMT_TEXTURE_PIF4_SWIZZLED: {
			header.format = 0x94;
			dest.write_n((u8*) texture->palette().data(), std::min(texture->palette().size(), (size_t) 16) * 4);
			for(size_t i = texture->palette().size(); i < 16; i++) {
				dest.write<u32>(0);
			}
			dest.write_n(texture->data.data(), texture->data.size());
			break;
		}
		case FMT_TEXTURE_PIF8:
		case FMT_TEXTURE_PIF8_SWIZZLED: {
			texture->swizzle_palette();
			
			header.format = 0x13;
			dest.write_n((u8*) texture->palette().data(), std::min(texture->palette().size(), (size_t) 256) * 4);
			for(size_t i = texture->palette().size(); i < 256; i++) {
				dest.write<u32>(0);
			}
			dest.write_n(texture->data.data(), texture->data.size());
			break;
		}
		default: assert(0);
	}
	
	dest.write(header_ofs, header);
}

static bool test_texture_asset(std::vector<u8>& original, std::vector<u8>& repacked, Game game, AssetFormatHint hint) {
	switch(hint) {
		case FMT_TEXTURE_PIF4:
		case FMT_TEXTURE_PIF4_SWIZZLED:
		case FMT_TEXTURE_PIF8:
		case FMT_TEXTURE_PIF8_SWIZZLED:
			assert(original.size() >= 8);
			*(u32*) &original[4] = 0;
			assert(repacked.size() >= 8);
			*(u32*) &repacked[4] = 0;
			original.resize(repacked.size());
			break;
		default: {} // Do nothing.
	}
	return false;
}
