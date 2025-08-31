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

#include <cppparser/cpp_parser.h>
#include <instancemgr/instances.h>
#include <assetmgr/asset_types.h>
#include <gui/render_mesh.h>
#include <editor/editor.h>

struct EditorClass
{
	Opt<Mesh> mesh;
	Opt<RenderMesh> render_mesh;
	std::vector<RenderMaterial> materials;
	Opt<RenderMaterial> icon;
	const CppType* pvar_type = nullptr;
};

struct EditorChunk
{
	RenderMesh collision;
	std::vector<RenderMesh> hero_collision;
	std::vector<RenderMaterial> collision_materials;
	RenderMesh tfrags;
};

class Level : public Editor<Level>
{
public:
	Level();
	
	void read(LevelAsset& asset, Game g);
	std::string save() override;
	
	LevelAsset& level();
	LevelWadAsset& level_wad();
	
	Instances& instances();
	const Instances& instances() const;
	
	Game game;
	
	std::vector<EditorChunk> chunks;
	std::vector<RenderMaterial> tfrag_materials;
	std::map<s32, EditorClass> moby_classes;
	std::map<s32, EditorClass> tie_classes;
	std::map<s32, EditorClass> shrub_classes;
	std::map<s32, EditorClass> camera_classes;
	std::map<s32, EditorClass> sound_classes;
	
private:
	LevelAsset* m_asset = nullptr;
	InstancesAsset* m_instances_asset = nullptr;
	Instances m_instances;
};

Opt<EditorClass> load_moby_editor_class(const MobyClassAsset& moby);
Opt<EditorClass> load_tie_editor_class(const TieClassAsset& tie);
Opt<EditorClass> load_shrub_editor_class(const ShrubClassAsset& shrub);

#endif
