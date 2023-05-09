/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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

#ifndef EDITOR_LEVEL_H
#define EDITOR_LEVEL_H

#include <map>

#include <engine/gameplay.h>
#include <assetmgr/asset_types.h>
#include <gui/render_mesh.h>
#include <editor/editor.h>

struct EditorClass {
	Mesh mesh;
	RenderMesh render_mesh;
	std::vector<RenderMaterial> materials;
};

struct EditorChunk {
	std::vector<RenderMesh> collision;
	std::vector<RenderMaterial> collision_materials;
	RenderMesh tfrags;
};

class Level : public Editor<Level> {
public:
	Level();
	
	void read(LevelAsset& asset, Game g);
	void save(const fs::path& path) override;
	
	LevelAsset& level();
	LevelWadAsset& level_wad();
	
	Gameplay& gameplay();
	
	Game game;
	
	std::vector<EditorChunk> chunks;
	std::vector<RenderMaterial> tfrag_materials;
	std::map<s32, EditorClass> mobies;
	std::map<s32, EditorClass> ties;
	std::map<s32, EditorClass> shrubs;
	
private:
	LevelAsset* _asset = nullptr;
	InstancesAsset* _gameplay_asset = nullptr;
	Gameplay _gameplay;
	PvarTypes _pvar_types;
};

#endif
