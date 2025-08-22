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

static s32 map_pixel_index_rac4(s32 i, s32 width);
static u8 map_palette_index(u8 index);

Texture::Texture() {}

Texture Texture::create_rgba(s32 width, s32 height, std::vector<u8> data)
{
	verify_fatal(data.size() == width * height * 4);
	
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::RGBA;
	texture.data = std::move(data);
	return texture;
}

Texture Texture::create_grayscale(s32 width, s32 height, std::vector<u8> data)
{
	verify_fatal(data.size() == width * height);
	
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::GRAYSCALE;
	texture.data = std::move(data);
	return texture;
}

Texture Texture::create_4bit_paletted(
	s32 width, s32 height, std::vector<u8> data, std::vector<u32> palette)
{
	verify_fatal(data.size() == (width * height / 2));
	verify_fatal(palette.size() <= 16);
	
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::PALETTED_4;
	texture.data = std::move(data);
	texture.palette() = std::move(palette);
	return texture;
}

Texture Texture::create_8bit_paletted(
	s32 width, s32 height, std::vector<u8> data, std::vector<u32> palette)
{
	verify_fatal(data.size() == width * height);
	verify_fatal(palette.size() <= 256);
	
	Texture texture;
	texture.width = width;
	texture.height = height;
	texture.format = PixelFormat::PALETTED_8;
	texture.data = std::move(data);
	texture.palette() = std::move(palette);
	return texture;
}

s32 Texture::bits_per_component() const
{
	switch (format) {
		case PixelFormat::RGBA: return 8;
		case PixelFormat::PALETTED_4: return 4;
		case PixelFormat::PALETTED_8: return 8;
		case PixelFormat::GRAYSCALE: return 8;
	}
	verify_fatal(0);
}

s32 Texture::bits_per_pixel() const
{
	switch (format) {
		case PixelFormat::RGBA: return 32;
		case PixelFormat::PALETTED_4: return 4;
		case PixelFormat::PALETTED_8: return 8;
		case PixelFormat::GRAYSCALE: return 8;
	}
	verify_fatal(0);
}

std::vector<u32>& Texture::palette()
{
	verify_fatal(format == PixelFormat::PALETTED_4 || format == PixelFormat::PALETTED_8)
	return m_palette;
}

const std::vector<u32>& Texture::palette() const
{
	verify_fatal(format == PixelFormat::PALETTED_4 || format == PixelFormat::PALETTED_8)
	return m_palette;
}

void Texture::to_rgba()
{
	std::vector<u8> rgba(width * height * 4);
	switch (format) {
		case PixelFormat::RGBA: {
			break;
		}
		case PixelFormat::PALETTED_4: {
			
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[(y * width + x) / 2];
					if (x % 2 == 0) {
						index >>= 4;
					} else {
						index &= 0xf;
					}
					*(u32*) &rgba[(y * width + x) * 4] = palette().at(index);
				}
			}
			data = std::move(rgba);
			break;
		}
		case PixelFormat::PALETTED_8: {
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[y * width + x];
					*(u32*) &rgba[(y * width + x) * 4] = palette().at(index);
				}
			}
			data = std::move(rgba);
			break;
		}
		case PixelFormat::GRAYSCALE: {
			verify_not_reached("Conversion from grayscale not yet implemented.");
			break;
		}
	}
	format = PixelFormat::RGBA;
}

void Texture::to_grayscale()
{
	std::vector<u8> grays(width * height * 4);
	switch (format) {
		case PixelFormat::RGBA: {
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u32 colour = *(u32*) &data[(y * height + x) * 4];
					u8 r = colour & 0x000000ff;
					u8 g = (colour & 0x0000ff00) >> 8;
					u8 b = (colour & 0x00ff0000) >> 16;
					grays[y * height + x] = (r + g + b) / 3;
				}
			}
			data = std::move(grays);
			break;
		}
		case PixelFormat::PALETTED_4: {
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[(y * width + x) / 2];
					if (x % 2 == 0) {
						index >>= 4;
					} else {
						index &= 0xf;
					}
					u8 colour = palette().at(index);
					u8 r = colour & 0x000000ff;
					u8 g = (colour & 0x0000ff00) >> 8;
					u8 b = (colour & 0x00ff0000) >> 16;
					grays[y * height + x] = (r + g + b) / 3;
				}
			}
			data = std::move(grays);
			break;
		}
		case PixelFormat::PALETTED_8: {
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[y * width + x];
					u8 colour = palette().at(index);
					u8 r = colour & 0x000000ff;
					u8 g = (colour & 0x0000ff00) >> 8;
					u8 b = (colour & 0x00ff0000) >> 16;
					grays[y * height + x] = (r + g + b) / 3;
				}
			}
			data = std::move(grays);
			break;
		}
		case PixelFormat::GRAYSCALE: {
			// Do nothing.
			break;
		}
	}
	format = PixelFormat::GRAYSCALE;
}

void Texture::to_4bit_paletted()
{
	switch (format) {
		case PixelFormat::RGBA: {
			verify_not_reached("Automatic palettization not yet implemented.");
			break;
		}
		case PixelFormat::PALETTED_4: {
			// Do nothing.
			break;
		}
		case PixelFormat::PALETTED_8: {
			std::vector<u8> indices(width * height);
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[y * width + x] & 0xf;
					if (x % 2 == 0) {
						indices[(y * width + x) / 2] = index << 4;
					} else {
						indices[(y * width + x) / 2] |= index;
					}
				}
			}
			data = std::move(indices);
			break;
		}
		case PixelFormat::GRAYSCALE: {
			verify_not_reached("Conversion from grayscale not yet implemented.");
			break;
		}
	}
	format = PixelFormat::PALETTED_4;
}

void Texture::to_8bit_paletted()
{
	switch (format) {
		case PixelFormat::RGBA: {
			verify_not_reached("Automatic palettization not yet implemented.");
			break;
		}
		case PixelFormat::PALETTED_4: {
			std::vector<u8> indices(width * height);
			for (s32 y = 0; y < height; y++) {
				for (s32 x = 0; x < width; x++) {
					u8 index = data[(y * width + x) / 2];
					if (x % 2 == 0) {
						index >>= 4;
					} else {
						index &= 0xf;
					}
					indices[y * width + x] = index;
				}
			}
			data = std::move(indices);
			break;
		}
		case PixelFormat::PALETTED_8: {
			// Do nothing.
			break;
		}
		case PixelFormat::GRAYSCALE: {
			verify_not_reached("Conversion from grayscale not yet implemented.");
			break;
		}
	}
	format = PixelFormat::PALETTED_8;
}

void Texture::reswizzle()
{
	switch (format) {
		case PixelFormat::PALETTED_4: {
			verify_not_reached("Swizzling this type of texture not yet implemented.");
			break;
		}
		case PixelFormat::PALETTED_8: {
			std::vector<u8> swizzled(data.size());
			for (s32 i = 0; i < data.size(); i++) {
				s32 map = map_pixel_index_rac4(i, width);
				if (map >= data.size()) {
					map = data.size() - 1;
				}
				swizzled[i] = data[map];
			}
			data = std::move(swizzled);
			break;
		}
		default: {
			verify_not_reached("Can't swizzle this type of texture.");
		}
	}
}

void Texture::swizzle()
{
	switch (format) {
		case PixelFormat::PALETTED_4: {
			verify_not_reached("Swizzling this type of texture not yet implemented.");
			break;
		}
		case PixelFormat::PALETTED_8: {
			std::vector<u8> swizzled(data.size());
			for (s32 i = 0; i < data.size(); i++) {
				s32 map = map_pixel_index_rac4(i, width);
				if (map >= data.size()) {
					map = data.size() - 1;
				}
				swizzled[map] = data[i];
			}
			data = std::move(swizzled);
			break;
		}
		default: {
			verify_not_reached("Can't swizzle this type of texture.");
		}
	}
}

void Texture::swizzle_palette()
{
	std::vector<u32> original = palette();
	for (size_t i = 0; i < palette().size(); i++) {
		palette()[i] = original.at(map_palette_index(i));
	}
}

void Texture::multiply_alphas()
{
	switch (format) {
		case PixelFormat::RGBA:
		case PixelFormat::GRAYSCALE: {
			for (size_t i = 3; i < data.size(); i += 4) {
				u8& alpha = data[i];
				if (alpha < 0x80) {
					alpha *= 2;
				} else {
					alpha = 255;
				}
			}
			break;
		}
		case PixelFormat::PALETTED_4:
		case PixelFormat::PALETTED_8: {
			for (u32& colour : palette()) {
				u32 alpha = (colour & 0xff000000) >> 24;
				if (alpha < 0x80) {
					alpha *= 2;
				} else {
					alpha = 255;
				}
				colour = (colour & 0xffffff) | (alpha << 24);
			}
			break;
		}
	}
}

void Texture::divide_alphas(bool handle_80s)
{
	switch (format) {
		case PixelFormat::RGBA:
		case PixelFormat::GRAYSCALE: {
			for (size_t i = 3; i < data.size(); i += 4) {
				u8& alpha = data[i];
				if (handle_80s && alpha == 0xff) {
					alpha = 0x80;
				} else {
					alpha /= 2;
				}
			}
			break;
		}
		case PixelFormat::PALETTED_4:
		case PixelFormat::PALETTED_8: {
			for (u32& colour : palette()) {
				u32 alpha = (colour & 0xff000000) >> 24;
				if (handle_80s && alpha == 0xff) {
					alpha = 0x80;
				} else {
					alpha /= 2;
				}
				colour = (colour & 0xffffff) | (alpha << 24);
			}
			break;
		}
	}
}

TextureMipmaps Texture::generate_mipmaps(s32 max_mip_levels)
{
	Texture texture = *this;
	texture.to_8bit_paletted();
	
	verify((texture.width & (texture.width - 1)) == 0, "Texture width is not a power of two.");
	verify(texture.width >= 8, "Texture width is less than 8 pixels.");
	
	TextureMipmaps output;
	
	for (s32 level = 0; level < max_mip_levels; level++) {
		if (texture.width >= 8) {
			output.mip_levels++;
			output.mips[level] = texture.data;
			texture.reduce();
		} else {
			break;
		}
	}
	
	// For now we use the same palette as the original texture.
	output.palette = m_palette;
	
	return output;
}

void Texture::reduce()
{
	std::vector<u8> reduced((width * height) / 4);
	for (s32 y = 0; y < (height / 2); y++) {
		for (s32 x = 0; x < (width / 2); x++) {
			reduced[y * (width / 2) + x] = data[(y * 2) * width + (x * 2)];
		}
	}
	data = std::move(reduced);
	width /= 2;
	height /= 2;
}

void Texture::destroy()
{
	width = 0;
	height = 0;
	data.clear();
	m_palette.clear();
}

bool Texture::operator<(const Texture& rhs) const
{
	if (width != rhs.width) return width < rhs.width;
	if (height != rhs.height) return height < rhs.height;
	if (format != rhs.format) return format < rhs.format;
	if (data != rhs.data) return data < rhs.data;
	return m_palette < rhs.m_palette;
}

bool Texture::operator==(const Texture& rhs) const
{
	return width == rhs.width
		&& height == rhs.height
		&& format == rhs.format
		&& data == rhs.data
		&& m_palette == rhs.m_palette;
}

static s32 map_pixel_index_rac4(s32 i, s32 width)
{
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

static u8 map_palette_index(u8 index)
{
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}
