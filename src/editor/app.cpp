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

#include <toolwads/wads.h>
#include <gui/gui.h>
#include "renderer.h"
#include "fs_includes.h"

Level* app::get_level()
{
	return m_lvl ? &(*m_lvl) : nullptr;
}

const Level* app::get_level() const
{
	return m_lvl ? &(*m_lvl) : nullptr;
}

BaseEditor* app::get_editor()
{
	return get_level();
}

void app::load_level(LevelAsset& asset)
{
	m_lvl.emplace();
	m_lvl->read(asset, game);
	reset_camera(this);
}

bool app::has_camera_control()
{
	return render_settings.camera_control;
}

GlTexture load_icon(s32 index)
{
	u8 buffer[32][16];
	g_editorwad.seek(wadinfo.editor.tool_icons[index].offset.bytes());
	g_editorwad.read_n((u8*) buffer, sizeof(buffer));
	
	uint32_t image_buffer[32][32];
	for (s32 y = 0; y < 32; y++) {
		for (s32 x = 0; x < 32; x++) {
			u8 gray;
			if (x % 2 == 0) {
				gray = ((buffer[y][x / 2] & 0xf0) >> 4) * 17;
			} else {
				gray = ((buffer[y][x / 2] & 0x0f) >> 0) * 17;
			}
			u8 alpha = (gray > 0) ? 0xff : 0x00;
			image_buffer[y][x] = gray | (gray << 8) | (gray << 16) | (alpha << 24);
		}
	}
	
	GlTexture texture;
	glGenTextures(1, &texture.id);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 32, 32, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_buffer);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	
	return texture;
}

app* g_app = nullptr;
FileInputStream g_editorwad = {};
