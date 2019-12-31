/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef FORMATS_LEVEL_TEXTURE_H
#define FORMATS_LEVEL_TEXTURE_H

#include <array>
#include <vector>
#include <glm/glm.hpp>

#include "../stream.h"
#include "../texture.h"
#include "../worker_logger.h"
#include "wad.h"
#include "racpak.h"

# /*
#	A texture stored using a stream. The member function wrap read/write calls.
#	Additionally, fip_scanner can scan game data files for 2FIP textures.
# */

class texture_provider;

class texture_impl : public texture {
public:
	struct offsets {
		std::size_t palette, pixels, width, height;
	};
	
	texture_impl(stream* backing, offsets offsets_);

	vec2i size() const override;
	void set_size(vec2i size_) override;

	std::array<colour, 256> palette() const override;
	void set_palette(std::array<colour, 256> palette_) override;

	std::vector<uint8_t> pixel_data() const override;
	void set_pixel_data(std::vector<uint8_t> pixel_data_) override;
	
	std::string palette_path() const override;
	std::string pixel_data_path() const override;

private:
	proxy_stream _backing;
	offsets _offsets;
};

class level_texture_provider : public texture_provider {
public:
	struct fmt {
		packed_struct(texture_entry,
			uint32_t unknown1;
			uint16_t width;
			uint16_t height;
			uint32_t unknown2;
			uint32_t pixel_data;
		)
	};

	level_texture_provider(stream* backing, std::string display_name_);
	level_texture_provider(const level_texture_provider&) = delete;

	std::string display_name() const override;
	std::vector<texture*> textures() override;

private:
	std::vector<std::unique_ptr<texture_impl>> _textures;
	std::string _display_name;
};

class fip_texture : public texture {
public:
	fip_texture(
		stream* backing,
		std::size_t offset,
		std::size_t size);

	vec2i size() const;
	void set_size(vec2i size_);

	std::array<colour, 256> palette() const override;
	void set_palette(std::array<colour, 256> palette_) override;

	std::vector<uint8_t> pixel_data() const override;
	void set_pixel_data(std::vector<uint8_t> pixel_data_) override;

	std::string palette_path() const override;
	std::string pixel_data_path() const override;

private:
	proxy_stream _backing;
};

class racpak_fip_scanner : public texture_provider {
public:
	racpak_fip_scanner(
		iso_stream* iso,
		racpak* archive,
		std::string display_name_,
		worker_logger& log);
	
	std::string display_name() const override;
	std::vector<texture*> textures() override;
private:
	std::vector<std::unique_ptr<fip_texture>> _textures;
	std::map<std::size_t, std::unique_ptr<array_stream>> _decompressed_streams;
	std::string _display_name;
};

#endif
