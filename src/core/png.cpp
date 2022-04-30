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

#include "png.h"

#include <png.h>

static void read_callback(png_structp png_ptr, png_bytep dest, size_t size);
static void write_callback(png_structp png_ptr, png_bytep src, size_t size);

Opt<Texture> read_png(InputStream& src) {
	std::vector<u8> magic = src.read_multiple<u8>(8);
	if(!png_check_sig(magic.data(), 8)) {
		printf("warning: PNG file has invalid magic bytes.\n");
		return {};
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		nullptr, nullptr, nullptr);
	if(!png_ptr) {
		printf("warning: png_create_read_struct failed.\n");
		return {};
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		printf("warning: png_create_info_struct failed.\n");
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		return {};
	}
	
	if(setjmp(png_jmpbuf(png_ptr))) {
		printf("warning: Error handler invoked when reading PNG.\n");
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		return {};
	}
	
	png_set_read_fn(png_ptr, &src, read_callback);
	png_set_sig_bytes(png_ptr, 8);
	
	png_read_info(png_ptr, info_ptr);
	
	u32 width;
	u32 height;
	s32 bit_depth;
	s32 colour_type;
	s32 interlace_type;
	assert(png_get_IHDR(png_ptr, info_ptr,
		&width, &height,
		&bit_depth, &colour_type,
		&interlace_type, nullptr, nullptr) != 0);
	
	switch(colour_type) {
		case PNG_COLOR_TYPE_RGB: {
			break;
		}
		case PNG_COLOR_TYPE_RGBA: {
			break;
		}
		case PNG_COLOR_TYPE_GRAY: {
			verify(bit_depth == 8, "Grayscale PNG files must have a bit depth of 8.");
			
			assert(png_get_rowbytes(png_ptr, info_ptr) == width);
			std::vector<u8> data(width * height);
			std::vector<png_bytep> row_pointers(height);
			for(u32 y = 0; y < height; y++) {
				row_pointers[y] = &data[y * width];
			}
			png_read_image(png_ptr, row_pointers.data());
			png_read_end(png_ptr, info_ptr);
			
			png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
			
			return Texture::create_grayscale(width, height, std::move(data));
		}
		case PNG_COLOR_TYPE_PALETTE: {
			png_colorp palette_rgb;
			s32 num_palette;
			assert(png_get_PLTE(png_ptr, info_ptr, &palette_rgb, &num_palette) != 0);
			assert(num_palette <= 256);
			
			png_bytep trans_alpha;
			s32 num_trans = 0;
			png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, nullptr);
			
			std::vector<u32> palette_rgba(num_palette);
			for(s32 i = 0; i < num_palette; i++) {
				u32& dest = palette_rgba[i];
				dest |= palette_rgb[i].red << 0;
				dest |= palette_rgb[i].green << 8;
				dest |= palette_rgb[i].blue << 16;
				if(i < num_trans) {
					dest |= trans_alpha[i] << 24;
				} else {
					dest |= 0xff000000;
				}
			}
			
			assert(png_get_rowbytes(png_ptr, info_ptr) == width);
			
			std::vector<u8> data;
			if(bit_depth == 4) {
				data.resize(width * height);
			} else if(bit_depth == 8) {
				data.resize(width * height);
			} else {
				printf("warning: PNG has invalid bit depth.");
				return {};
			}
			std::vector<png_bytep> row_pointers(height);
			for(u32 y = 0; y < height; y++) {
				row_pointers[y] = &data[(y * width) / ((bit_depth == 4) ? 2 : 1)];
			}
			png_read_image(png_ptr, row_pointers.data());
			
			png_read_end(png_ptr, info_ptr);
			png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
			
			return Texture::create_8bit_paletted(width, height, std::move(data), std::move(palette_rgba));
		}
	}
	
	printf("warning: PNG file has wrong format.");
	return {};
}

static void read_callback(png_structp png_ptr, png_bytep dest, size_t size) {
	png_voidp io_ptr = png_get_io_ptr(png_ptr);
	assert(io_ptr);
	InputStream& src = *(InputStream*) io_ptr;
	src.read(dest, size);
}

void write_png(OutputStream& dest, const Texture& texture) {
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	verify(png_ptr, "png_create_write_struct failed.");
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	verify(info_ptr, "png_create_info_struct failed.");
	
	if(setjmp(png_jmpbuf(png_ptr))) {
		verify_not_reached("Failed to encode PNG file.");
	}
	
	png_set_write_fn(png_ptr, &dest, write_callback, nullptr);
	
	switch(texture.format) {
		case PixelFormat::RGBA: {
			verify_not_reached("Support for writing RGBA PNG files not yet implemented.");
			break;
		}
		case PixelFormat::GRAYSCALE: {
			assert(texture.data.size() == texture.width * texture.height);
			
			png_set_IHDR(png_ptr, info_ptr,
				texture.width, texture.height, 8,
				PNG_COLOR_TYPE_GRAY, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			
			png_write_info(png_ptr, info_ptr);
			
			std::vector<png_bytep> row_pointers(texture.height);
			for(s32 y = 0; y < texture.height; y++) {
				s32 row_offset = y * texture.width;
				row_pointers[y] = const_cast<u8*>(&texture.data[y * texture.width]);
			}
			
			png_write_image(png_ptr, row_pointers.data());
			
			break;
		}
		case PixelFormat::PALETTED_4: {
			assert(texture.data.size() == (texture.width * texture.height) / 2);
			
			png_set_IHDR(png_ptr, info_ptr,
				texture.width, texture.height, 4,
				PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			
			assert(texture.palette().size() <= 16);
			png_color palette[16];
			for(s32 i = 0; i < texture.palette().size(); i++) {
				u32 colour = texture.palette()[i];
				palette[i].red   = (colour & 0x000000ff) >> 0;
				palette[i].green = (colour & 0x0000ff00) >> 8;
				palette[i].blue  = (colour & 0x00ff0000) >> 16;
			}
			png_set_PLTE(png_ptr, info_ptr, palette, texture.palette().size());
			
			png_byte alphas[16];
			for(s32 i = 0; i < texture.palette().size(); i++) {
				alphas[i] = (texture.palette()[i] & 0xff000000) >> 24;
			}
			png_set_tRNS(png_ptr, info_ptr, alphas, texture.palette().size(), nullptr);
			
			png_write_info(png_ptr, info_ptr);
			
			std::vector<png_bytep> row_pointers(texture.height);
			for(s32 y = 0; y < texture.height; y++) {
				s32 row_offset = (y * texture.width) / 2;
				row_pointers[y] = (png_bytep) texture.data.data() + row_offset;
			}
			
			png_write_image(png_ptr, row_pointers.data());
			
			break;
		}
		case PixelFormat::PALETTED_8: {
			assert(texture.data.size() == texture.width * texture.height);
			
			png_set_IHDR(png_ptr, info_ptr,
				texture.width, texture.height, 8,
				PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE,
				PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
			
			assert(texture.palette().size() <= 256);
			png_color palette[256];
			for(s32 i = 0; i < texture.palette().size(); i++) {
				u32 colour = texture.palette()[i];
				palette[i].red   = (colour & 0x000000ff) >> 0;
				palette[i].green = (colour & 0x0000ff00) >> 8;
				palette[i].blue  = (colour & 0x00ff0000) >> 16;
			}
			png_set_PLTE(png_ptr, info_ptr, palette, texture.palette().size());
			
			png_byte alphas[256];
			for(s32 i = 0; i < texture.palette().size(); i++) {
				alphas[i] = (texture.palette()[i] & 0xff000000) >> 24;
			}
			png_set_tRNS(png_ptr, info_ptr, alphas, texture.palette().size(), nullptr);
			
			png_write_info(png_ptr, info_ptr);
			
			std::vector<png_bytep> row_pointers(texture.height);
			for(s32 y = 0; y < texture.height; y++) {
				s32 row_offset = y * texture.width;
				row_pointers[y] = (png_bytep) texture.data.data() + row_offset;
			}
			
			png_write_image(png_ptr, row_pointers.data());
			
			break;
		}
	}
	
	png_write_end(png_ptr, info_ptr);
	png_destroy_write_struct(&png_ptr, &info_ptr);
}

static void write_callback(png_structp png_ptr, png_bytep src, size_t size) {
	png_voidp io_ptr = png_get_io_ptr(png_ptr);
	assert(io_ptr);
	OutputStream& dest = *(OutputStream*) io_ptr;
	dest.write(src, size);
}
