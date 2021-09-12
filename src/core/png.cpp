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

Opt<Texture> read_png(const char* file_name) {
	FILE* file = fopen(file_name, "rb");
	if(!file) {
		printf("warning: Failed to open %s for reading.\n", file_name);
		return {};
	}
	
	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,
		nullptr, nullptr, nullptr);
	if(!png_ptr) {
		printf("warning: png_create_read_struct failed (%s).\n", file_name);
		fclose(file);
		return {};
	}
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	if(!info_ptr) {
		printf("warning: png_create_info_struct failed (%s).\n", file_name);
		png_destroy_read_struct(&png_ptr, nullptr, nullptr);
		fclose(file);
		return {};
	}
	
	if(setjmp(png_jmpbuf(png_ptr))) {
		printf("warning: Error handler invoked when reading PNG (%s).\n", file_name);
		png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
		fclose(file);
		return {};
	}
	
	png_init_io(png_ptr, file);
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
	
	png_colorp palette;
	s32 num_palette;
	assert(png_get_PLTE(png_ptr, info_ptr, &palette, &num_palette) != 0);
	assert(num_palette <= 256);
	
	png_bytep trans_alpha;
	s32 num_trans = 0;
	png_get_tRNS(png_ptr, info_ptr, &trans_alpha, &num_trans, nullptr);
	
	Texture texture = {0};
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::IDTEX8;
	for(s32 i = 0; i < num_palette; i++) {
		u32& dest = texture.palette.colours[i];
		dest |= palette[i].red << 0;
		dest |= palette[i].green << 8;
		dest |= palette[i].blue << 16;
		if(i < num_trans) {
			dest |= trans_alpha[i] << 24;
		} else {
			dest |= 0xff000000;
		}
	}
	texture.palette.top = num_palette;
	
	assert(png_get_rowbytes(png_ptr, info_ptr) == width);
	texture.pixels.resize(width * height);
	std::vector<png_bytep> row_pointers(height);
	for(u32 y = 0; y < height; y++) {
		row_pointers[y] = &texture.pixels[y * width];
	}
	png_read_image(png_ptr, row_pointers.data());
	png_read_end(png_ptr, info_ptr);
	
	png_destroy_read_struct(&png_ptr, &info_ptr, nullptr);
	fclose(file);
	
	return texture;
}

void write_png(const char* file_name, const Texture& texture) {
	FILE* file = fopen(file_name, "wb");
	verify(file, "Failed to open %s for writing.", file_name);
	
	png_structp png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	verify(png_ptr, "png_create_write_struct failed.");
	
	png_infop info_ptr = png_create_info_struct(png_ptr);
	verify(info_ptr, "png_create_info_struct failed.");
	
	if(setjmp(png_jmpbuf(png_ptr))) {
		verify_not_reached("Failed to encode PNG file (%s).", file_name);
	}
	
	png_init_io(png_ptr, file);
	
	png_set_IHDR(png_ptr, info_ptr,
		texture.width, texture.height, 8,
		PNG_COLOR_TYPE_PALETTE, PNG_INTERLACE_NONE, 
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	
	assert(texture.palette.top <= 256);
	png_color palette[256];
	for(s32 i = 0; i < texture.palette.top; i++) {
		u32 colour = texture.palette.colours[i];
		palette[i].red   = (colour & 0x000000ff) >> 0;
		palette[i].green = (colour & 0x0000ff00) >> 8;
		palette[i].blue  = (colour & 0x00ff0000) >> 16;
	}
	png_set_PLTE(png_ptr, info_ptr, palette, texture.palette.top);
	
	png_byte alphas[256];
	for(s32 i = 0; i < texture.palette.top; i++) {
		alphas[i] = (texture.palette.colours[i] & 0xff000000) >> 24;
	}
	png_set_tRNS(png_ptr, info_ptr, alphas, texture.palette.top, nullptr);
	
	png_write_info(png_ptr, info_ptr);
	
	std::vector<png_bytep> row_pointers(texture.height);
	for(s32 y = 0; y < texture.height; y++) {
		s32 row_offset = y * texture.width;
		row_pointers[y] = (png_bytep) texture.pixels.data() + row_offset;
	}
	
	png_write_image(png_ptr, row_pointers.data());
	png_write_end(png_ptr, info_ptr);
	
	png_destroy_write_struct(&png_ptr, &info_ptr);
	fclose(file);
}
