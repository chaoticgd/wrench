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

#include "collision_fixer.h"

#include <core/worker_thread.h>
#include <engine/collision.h>
#include <gui/gui.h>
#include <editor/app.h>
#include <editor/instanced_collision_recovery.h>
#include <editor/gui/asset_selector.h>

class CollisionFixerThread : public WorkerThread {
public:
	void start(Game game, std::string game_bank_path, s32 type, s32 o_class);
	void run() override;
	void clear() override;
	
	// Input
	Game game;
	std::string game_bank_path;
	s32 type;
	s32 o_class;
	
	// Output
	bool success = false;
	Mesh mesh;
	
	Opt<Mesh> get_output();
	
private:
	bool loaded = false;
	AssetForest forest;
	AssetBank* bank = nullptr;
	std::vector<ColLevel> levels;
	ColMappings mappings;
};

static CollisionFixerThread fixer_thread;
static EditorClass preview_class;
static RenderMesh collision_mesh;
static std::vector<RenderMaterial> collision_materials;

static std::pair<s32, s32> class_selector();

void collision_fixer() {
	bool params_modified = false;
	
	auto [type, o_class] = class_selector();
	if(type != -1 && o_class != -1) {
		params_modified = true;
	}
	
	ImGui::Text("Parameters");
	
	
	static float quant_factor = 4.f;
	params_modified |= ImGui::SliderFloat("Quantization Factor", &quant_factor, 0.1f, 100.f);
	
	static int min_hits = 3;
	params_modified |= ImGui::SliderInt("Minimum Hits", &min_hits, 1, 100);
	
	static float discard_distance = 100.f;
	params_modified |= ImGui::SliderFloat("Discard Distance", &discard_distance, 0.1f, 100.f);
	
	static float merge_distance = 0.01f;
	params_modified |= ImGui::SliderFloat("Merge Distance", &merge_distance, 0.f, 0.1f);
	
	ImGui::Text("Subtraction");
	
	static bool subtract = false;
	ImGui::Checkbox("Prune World-Space Collision", &subtract);
	
	static float subtraction_distance;
	ImGui::SliderFloat("Subtraction Distance", &subtraction_distance, 0.f, 1.f);
	
	ImGui::Text("Previews");
	
	ImGui::SliderFloat("Zoom", &g_app->collision_fixer_previews.params.zoom, 0.f, 1.f);
	
	if(ImGui::Button("Write Collision Mesh")) {
		
	}
	
	ImGui::Text("Thread State: %s", fixer_thread.state_string());
	
	static bool has_run = false;
	static bool waiting_for_completion = false;
	if(!has_run || params_modified) {
		g_app->collision_fixer_previews.collision_mesh = nullptr;
		g_app->collision_fixer_previews.collision_materials = nullptr;
		
		fixer_thread.interrupt();
		fixer_thread.start(g_app->game, g_app->game_path, type, o_class);
		has_run = true;
		waiting_for_completion = true;
	}
	
	if(waiting_for_completion && !fixer_thread.is_running()) {
		Opt<Mesh> out = fixer_thread.get_output();
		if(out.has_value()) {
			collision_mesh = upload_mesh(*out, true);
			Texture white = Texture::create_rgba(1, 1, {0xff, 0xff, 0xff, 0xff});
			RenderMaterial mat = upload_material(Material{"", glm::vec4(1.f, 1.f, 1.f, 1.f)}, {white});
			collision_materials.clear();
			collision_materials.emplace_back(std::move(mat));
			printf("done\n");
			g_app->collision_fixer_previews.collision_mesh = &collision_mesh;
			g_app->collision_fixer_previews.collision_materials = &collision_materials;
			waiting_for_completion = false;
		}
	}
}

void CollisionFixerThread::start(Game game, std::string game_bank_path, s32 type, s32 o_class) {
	printf("start()\n");
	state = STARTING;
	CollisionFixerThread* command = this;
	thread = std::thread([command, game, game_bank_path, type, o_class]() {
		command->game = game;
		command->game_bank_path = game_bank_path;
		command->type = type;
		command->o_class = o_class;
		{
			std::lock_guard<std::mutex> lock(command->mutex);
			command->state = RUNNING;
		}
		command->run();
	});
}

void CollisionFixerThread::run() {
	printf("run()\n");
	success = false;
	
	auto check_is_still_running = [&]() {
		std::lock_guard<std::mutex> g(mutex);
		return state == RUNNING;
	};
	
	static bool loaded = false;
	if(!loaded) {
		printf("build!!!!\n");
		bank = &forest.mount<LooseAssetBank>(game_bank_path, false);
		BuildAsset& build = bank->root()->get_child(game_to_string(game).c_str()).as<BuildAsset>();
		levels = load_instance_collision_data(build, check_is_still_running);
		if(!check_is_still_running()) {
			std::lock_guard<std::mutex> g(mutex);
			state = STOPPED;
			success = false;
			printf("interrupted\n");
			return;
		}
		mappings = generate_instance_collision_mappings(levels);
		loaded = true;
	}
	
	if(!check_is_still_running()) {
		std::lock_guard<std::mutex> g(mutex);
		state = STOPPED;
		success = false;
		printf("interrupted\n");
		return;
	}
	
	Opt<Mesh> m = build_instanced_collision(type, o_class, mappings, levels, check_is_still_running);
	if(m.has_value()) {
		mesh = std::move(*m);
		success = true;
		printf("-> success\n");
	} else {
		mesh = {};
		success = false;
		printf("-> fail\n");
	}
	
	transition_to_stopped([&](){});
}

void CollisionFixerThread::clear() {
	game_bank_path.clear();
	success = false;
	mesh = {};
}

Opt<Mesh> CollisionFixerThread::get_output() {
	std::lock_guard<std::mutex> g(mutex);
	if(success) {
		success = false;
		return std::move(mesh);
	} else {
		return std::nullopt;
	}
}

static std::pair<s32, s32> class_selector() {
	static AssetSelector tie_selector, shrub_selector;
	tie_selector.required_type = TieClassAsset::ASSET_TYPE;
	tie_selector.omit_type = LevelAsset::ASSET_TYPE;
	shrub_selector.required_type = ShrubClassAsset::ASSET_TYPE;
	shrub_selector.omit_type = LevelAsset::ASSET_TYPE;
	
	s32 type = -1;
	s32 o_class = -1;
	if(ImGui::BeginTabBar("class_type_bar")) {
		if(ImGui::BeginTabItem("Tie Class")) {
			if(Asset* asset = asset_selector("##tie_selector", "(select tie class)", tie_selector, g_app->asset_forest)) {
				TieClassAsset& tie = asset->as<TieClassAsset>();
				Opt<EditorClass> ec = load_tie_editor_class(tie);
				if(ec.has_value() && ec->render_mesh.has_value()) {
					preview_class = std::move(*ec);
					g_app->collision_fixer_previews.mesh = &(*preview_class.render_mesh);
					g_app->collision_fixer_previews.materials = &preview_class.materials;
				} else {
					g_app->collision_fixer_previews.mesh = nullptr;
					g_app->collision_fixer_previews.materials = nullptr;
				}
				type = COL_TIE;
				o_class = tie.id();
			}
			ImGui::EndTabItem();
		}
		if(ImGui::BeginTabItem("Shrub Class")) {
			if(Asset* asset = asset_selector("##shrub_selector", "(select shrub class)", shrub_selector, g_app->asset_forest)) {
				ShrubClassAsset& shrub = asset->as<ShrubClassAsset>();
				Opt<EditorClass> ec = load_shrub_editor_class(shrub);
				if(ec.has_value() && ec->render_mesh.has_value()) {
					preview_class = std::move(*ec);
					g_app->collision_fixer_previews.mesh = &(*preview_class.render_mesh);
					g_app->collision_fixer_previews.materials = &preview_class.materials;
				} else {
					g_app->collision_fixer_previews.mesh = nullptr;
					g_app->collision_fixer_previews.materials = nullptr;
				}
				type = COL_SHRUB;
				o_class = shrub.id();
			}
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}
	
	return {type, o_class};
}
