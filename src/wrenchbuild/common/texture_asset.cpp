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
#include <wrenchbuild/asset_unpacker.h>
#include <wrenchbuild/asset_packer.h>
#include <wrenchbuild/tests.h>

static void unpack_texture_asset(
	TextureAsset& dest, InputStream& src, BuildConfig config, const char* hint);
static void pack_texture_asset(
	OutputStream& dest, const TextureAsset& src, BuildConfig config, const char* hint);
static Texture unpack_pif (InputStream& src);
static void pack_pif (OutputStream& dest, Texture& texture, s32 mip_level);
static bool test_texture_asset(
	std::vector<u8>& original,
	std::vector<u8>& repacked,
	BuildConfig config,
	const char* hint,
	AssetTestMode mode);

on_load(Texture, []() {
	TextureAsset::funcs.unpack_rac1 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_rac2 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_rac3 = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	TextureAsset::funcs.unpack_dl = wrap_hint_unpacker_func<TextureAsset>(unpack_texture_asset);
	
	TextureAsset::funcs.pack_rac1 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_rac2 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_rac3 = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	TextureAsset::funcs.pack_dl = wrap_hint_packer_func<TextureAsset>(pack_texture_asset);
	
	TextureAsset::funcs.test_rac = wrap_diff_test_func(test_texture_asset);
	TextureAsset::funcs.test_gc  = wrap_diff_test_func(test_texture_asset);
	TextureAsset::funcs.test_uya = wrap_diff_test_func(test_texture_asset);
	TextureAsset::funcs.test_dl  = wrap_diff_test_func(test_texture_asset);
})

packed_struct(RgbaTextureHeader,
	s32 width;
	s32 height;
	u32 pad[2];
)

static void unpack_texture_asset(
	TextureAsset& dest, InputStream& src, BuildConfig config, const char* hint)
{
	Texture texture;
	
	const char* type = next_hint(&hint);
	if (strcmp(type, "rgba") == 0) {
		RgbaTextureHeader header = src.read<RgbaTextureHeader>(0);
		std::vector<u8> data = src.read_multiple<u8>(0x10, header.width * header.height * 4);
		texture = Texture::create_rgba(header.width, header.height, data);
		texture.multiply_alphas();
	} else if (strcmp(type, "rawrgba") == 0) {
		s32 width = atoi(next_hint(&hint));
		s32 height = atoi(next_hint(&hint));
		std::vector<u8> data = src.read_multiple<u8>(0, width * height * 4);
		texture = Texture::create_rgba(width, height, data);
		texture.multiply_alphas();
	} else if (strcmp(type, "pif") == 0) {
		next_hint(&hint); // palette_size
		next_hint(&hint); // mip_levels
		bool swizzled = strcmp(next_hint(&hint), "swizzled") == 0;
		texture = unpack_pif (src);
		if (swizzled) {
			texture.swizzle();
		}
	} else {
		verify_not_reached("Tried to unpack a texture with an invalid hint.");
	}
	
	// If we're unpacking a list of material assets, we use the tag of the
	// material e.g. "0", "1", "2" etc instead of the tag of the texture itself
	// e.g. "diffuse".
	std::string name;
	if (dest.parent() && dest.parent()->logical_type() == MaterialAsset::ASSET_TYPE) {
		name = dest.parent()->tag();
	} else {
		name = dest.tag();
	}
	
	auto [file, ref] = dest.file().open_binary_file_for_writing(name + ".png");
	write_png(*file, texture);
	dest.set_src(ref);
}

static void pack_texture_asset(
	OutputStream& dest, const TextureAsset& src, BuildConfig config, const char* hint)
{
	auto stream = src.src().open_binary_file_for_reading();
	verify(stream.get(), "Failed to open PNG file.");
	Opt<Texture> texture = read_png(*stream);
	verify(texture.has_value(), "Failed to read PNG file.");
	
	const char* type = next_hint(&hint);
	if (strcmp(type, "rgba") == 0) {
		texture->to_rgba();
		texture->divide_alphas(false);
		
		RgbaTextureHeader header = {0};
		header.width = texture->width;
		header.height = texture->height;
		dest.write(header);
		dest.write_v(texture->data);
	} else if (strcmp(type, "rawrgba") == 0) {
		s32 width = atoi(next_hint(&hint));
		s32 height = atoi(next_hint(&hint));
		texture->to_rgba();
		texture->divide_alphas();
		verify(texture->width == width && texture->height == height,
			"RGBA image has wrong size, should be %d by %d.",
			width, height);
		dest.write_v(texture->data);
	} else if (strcmp(type, "pif") == 0) {
		s32 palette_size = atoi(next_hint(&hint));
		if (palette_size == 4) {
			texture->to_4bit_paletted();
		} else if (palette_size == 8) {
			texture->to_8bit_paletted();
		} else {
			verify_not_reached("Tried to pack a texture with an invalid palette size specified in the hint.");
		}
		s32 mip_levels = atoi(next_hint(&hint));
		const char* swizzled = next_hint(&hint);
		// TODO: Once we've figure out swizzling for where palette_size == 4
		// make sure to enable that here.
		if (strcmp(swizzled, "swizzled") == 0 && palette_size == 8) {
			texture->swizzle();
		}
		pack_pif (dest, *texture, mip_levels);
	} else {
		verify_not_reached("Tried to pack a texture with an invalid hint.");
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

static Texture unpack_pif (InputStream& src)
{
	PifHeader header = src.read<PifHeader>(0);
	verify(memcmp(header.magic, "2FIP", 4) == 0, "PIF has bad magic bytes.");
	verify(header.width <= 2048 && header.height <= 2048, "PIF has bad width/height values.");
	
	switch (header.format) {
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

static void pack_pif (OutputStream& dest, Texture& texture, s32 mip_levels)
{
	texture.divide_alphas();
	
	s64 header_ofs = dest.tell();
	PifHeader header = {};
	dest.write(header);
	memcpy(header.magic, "2FIP", 4);
	header.width = texture.width;
	header.height = texture.height;
	header.mip_levels = 1;
	
	switch (texture.format) {
		case PixelFormat::PALETTED_4: {
			verify_fatal(texture.data.size() == (texture.width * texture.height) / 2);
			
			header.format = 0x94;
			dest.write_n((u8*) texture.palette().data(), std::min(texture.palette().size(), (size_t) 16) * 4);
			for (size_t i = texture.palette().size(); i < 16; i++) {
				dest.write<u32>(0);
			}
			dest.write_n(texture.data.data(), texture.data.size());
			break;
		}
		case PixelFormat::PALETTED_8: {
			texture.swizzle_palette();
			
			verify_fatal(texture.data.size() == texture.width * texture.height);
			
			TextureMipmaps mipmaps = texture.generate_mipmaps(mip_levels);
			
			header.format = 0x13;
			header.mip_levels = mipmaps.mip_levels;
			
			dest.write_n((u8*) mipmaps.palette.data(), std::min(mipmaps.palette.size(), (size_t) 256) * 4);
			for (size_t i = mipmaps.palette.size(); i < 256; i++) {
				dest.write<u32>(0);
			}
			for (s32 level = 0 ; level < mipmaps.mip_levels; level++) {
				dest.write_n(mipmaps.mips[level].data(), mipmaps.mips[level].size());
			}
			
			break;
		}
		default: verify_fatal(0);
	}
	
	dest.write(header_ofs, header);
}

static bool test_texture_asset(
	std::vector<u8>& original,
	std::vector<u8>& repacked,
	BuildConfig config,
	const char* hint,
	AssetTestMode mode)
{
	const char* type = next_hint(&hint);
	if (strcmp(type, "pif") == 0) {
		// We don't know what this field in the PIF header is and it doesn't
		// seem to be used, so just zero it during tests.
		verify_fatal(original.size() >= 8);
		*(u32*) &original[4] = 0;
		verify_fatal(repacked.size() >= 8);
		*(u32*) &repacked[4] = 0;
		original.resize(repacked.size());
	}
	return diff_buffers(original, repacked, 0, DIFF_REST_OF_BUFFER, mode == AssetTestMode::PRINT_DIFF_ON_FAIL);
}
