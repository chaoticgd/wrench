/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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
#include "../worker_logger.h"
#include "wad.h"
#include "racpak.h"
#include "texture.h"

# /*
#	A texture stored using a stream. The member function wrap read/write calls.
#	Additionally, fip_scanner can scan game data files for 2FIP textures.
# */

class texture_provider;


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
	std::vector<texture> _textures;
	std::string _display_name;
};

std::vector<texture> enumerate_fip_textures(iso_stream* iso, racpak* archive);

#endif
