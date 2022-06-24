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

#include "app.h"

#include <stdlib.h>
#include "unwindows.h"
#include <toml11/toml.hpp>

#include "gui/gui.h"
#include "renderer.h"
#include "fs_includes.h"

Level* app::get_level() {
	return _lvl ? &(*_lvl) : nullptr;
}

const Level* app::get_level() const {
	return _lvl ? &(*_lvl) : nullptr;
}

BaseEditor* app::get_editor() {
	return get_level();
}

void app::load_level(LevelAsset& asset) {
	_lvl.emplace();
	_lvl->read(asset, Game::GC);
	reset_camera(this);
}

bool app::has_camera_control() {
	return render_settings.camera_control;
}

GlTexture load_icon(std::string path) {
	std::ifstream image_file(path);
	
	uint32_t image_buffer[32][32];
	for(std::size_t y = 0; y < 32; y++) {
		std::string line;
		std::getline(image_file, line);
		if(line.size() > 32) {
			line = line.substr(0, 32);
		}
		for(std::size_t x = 0; x < line.size(); x++) {
			image_buffer[y][x] = line[x] == '#' ? 0xffffffff : 0x00000000;
		}
		for(std::size_t x = line.size(); x < 32; x++) {
			image_buffer[y][x] = 0;
		}
	}
	
	GlTexture texture;
	glGenTextures(1, &texture());
	glBindTexture(GL_TEXTURE_2D, texture());
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	return texture;
}

app* g_app = nullptr;
