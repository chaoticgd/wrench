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

#ifndef APP_H
#define APP_H

#include <any>
#include <set>
#include <atomic>
#include <time.h>
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>

#include <assetmgr/asset.h>
#include <gui/gui.h>
#include <editor/level.h>
#include <editor/tools.h>
#include <editor/renderer.h>
#include <editor/fs_includes.h>
#include <editor/gui/collision_fixer.h>

struct GLFWwindow;

class app
{
public:
	app() {}
	
	glm::vec2 mouse_last { 0, 0 };

	GLFWwindow* glfw_window;
	int window_width, window_height;
	
	std::string game_path;
	std::string overlay_path;
	std::string mod_path;
	
	AssetForest asset_forest;
	AssetBank* underlay_bank = nullptr;
	AssetBank* game_bank = nullptr;
	AssetBank* overlay_bank = nullptr;
	AssetBank* mod_bank = nullptr;
	Game game = Game::UNKNOWN;
	
	RenderSettings render_settings;
	CollisionFixerPreviews collision_fixer_previews;
	
	f32 delta_time = 0;
	bool last_frame = false;
	
	Level* get_level();
	const Level* get_level() const;
	
	BaseEditor* get_editor();
	
	void load_level(LevelAsset& asset);
	
private:
	std::optional<Level> m_lvl;

public:

	bool has_camera_control();
};

extern app* g_app;
extern FileInputStream g_editorwad;

GlTexture load_icon(s32 index);

#endif
