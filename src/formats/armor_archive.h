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

#ifndef FORMATS_ARMOR_ARCHIVE_H
#define FORMATS_ARMOR_ARCHIVE_H

#include <memory>
#include <string>
#include <vector>

#include "../stream.h"
#include "../texture.h"
#include "texture_impl.h"
#include "game_model.h"

# /*
# 	Read ARMOR.WAD.
# */

class armor_archive : public texture_provider {
public:
	struct fmt {
		packed_struct(header,
			uint32_t header_size;
			uint32_t pad;
		)
		
		packed_struct(armor,
			sector32 model;
			uint32_t model_size;
			sector32 texture;
			uint32_t texture_size;
		)
	};
	
	armor_archive(stream* backing, std::size_t offset, std::size_t size);
	std::string display_name() const override;
	std::string display_name_of(texture* tex) const override;
	std::vector<texture*> textures() override;
	
	std::vector<game_model> models;

private:
	proxy_stream _backing;
	std::vector<std::unique_ptr<fip_texture>> _textures;
	std::map<texture*, std::string> _texture_names;
};

#endif
