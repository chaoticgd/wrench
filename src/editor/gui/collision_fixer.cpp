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

#include <mutex>
#include <thread>
#include <assetmgr/asset_path_gen.h>
#include <gui/gui.h>
#include <editor/app.h>
#include <editor/instanced_collision_recovery.h>
#include <editor/gui/asset_selector.h>

class CollisionFixerThread
{
public:
	bool interrupt();
	void start(Game game, std::string game_bank_path, s32 type, s32 o_class, const ColParams& params);
	void run();
	void reset();
	
	bool is_running();
	const char* state_string();
	
	// Input
	Game game;
	std::string game_bank_path;
	s32 type;
	s32 o_class;
	ColParams params;
	
	// Output
	bool success = false;
	ColladaScene scene;
	
	Opt<ColladaScene> get_output();
	
private:
	enum ThreadState { // condition                                  description
		NOT_RUNNING,   // initial state, main thread sees STOPPED    the worker is not running (hasn't yet been spawned, or has been joined)
		STARTING,      // start() called on main thread              the worker thread hasn't started yet
		LOADING_DATA,  // worker is loading level data               the worker is loading level data
		RECOVERING,    // worker recovering collision                the worker is recovering collision
		STOPPING,      // stop() called on main thread               the main thread has requested the worker stop
		STOPPED,       // worker sees STOPPING or is finished        the worker has stopped, main thread needs to acknowledge
	};

	std::mutex m_mutex;
	std::thread m_thread;
	ThreadState m_state = NOT_RUNNING;
	
	bool m_loaded = false;
	AssetForest m_forest;
	AssetBank* m_bank = nullptr;
	std::vector<ColLevel> m_levels;
	ColMappings m_mappings;
};

static CollisionFixerThread fixer_thread;
static EditorClass preview_class;
static RenderMesh collision_render_mesh;
static std::vector<RenderMaterial> collision_materials;
static ColParams params;

static std::tuple<s32, s32, Asset*> class_selector();
static void generate_bounding_box(const Mesh& mesh);
static std::string write_instanced_collision(Asset& asset, const ColladaScene& collision_scene);
template <typename ThisAsset>
static std::string write_instanced_collision_for_class_of_type(ThisAsset& asset, const ColladaScene& collision_scene);

static void row(const char* name)
{
	ImGui::TableNextRow();
	ImGui::TableNextColumn();
	ImGui::AlignTextToFramePadding();
	ImGui::Text("%s", name);
	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(-1.f);
};

void collision_fixer()
{
	bool bb_modified = false;
	bool params_modified = false;
	
	static s32 type = -1, o_class = -1;
	static Asset* asset = nullptr;
	ImGui::PushStyleColor(ImGuiCol_FrameBg, 0);
	ImGui::PushStyleVar(ImGuiStyleVar_CellPadding, ImVec2(8, 8));
	if (ImGui::BeginTable("inspector", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable)) {
		ImGui::TableSetupColumn("name", ImGuiTableColumnFlags_WidthFixed | ImGuiTableColumnFlags_NoResize);
		ImGui::TableSetupColumn("input", ImGuiTableColumnFlags_WidthStretch);
		
		row("Asset");
		auto [t, oc, a] = class_selector();
		if (t != -1 && oc != -1) {
			type = t;
			o_class = oc;
			asset = a;
			bb_modified = true;
		}
		
		row("Threshold");
		static bool extend_threshold_range = false;
		params_modified |= ImGui::SliderInt("##threshold", &params.min_hits, 1, extend_threshold_range ? 100 : 10);
		row("Extend Threshold Range");
		ImGui::Checkbox("##extend_threshold_range", &extend_threshold_range);
		row("Merge Distance");
		params_modified |= ImGui::SliderFloat("##merge_dist", &params.merge_dist, 0.01f, 1.f, "%.2f");
		row("Reject Faces Outside BB");
		params_modified |= ImGui::Checkbox("##reject", &params.reject_faces_outside_bb);
		row("Bounding Box Origin");
		if (ImGui::InputFloat3("##bb_origin", &params.bounding_box_origin[0])) {
			bb_modified = true;
		}
		row("Bounding Box Size");
		if (ImGui::InputFloat3("##bb_size", &params.bounding_box_size[0])) {
			bb_modified = true;
		}
		
		if (bb_modified) {
			g_app->collision_fixer_previews.params.bounding_box_origin = params.bounding_box_origin;
			g_app->collision_fixer_previews.params.bounding_box_size = params.bounding_box_size;
			params_modified = true;
		}
		
		row("Preview Zoom");
		ImGui::SliderFloat("Preview Zoom", &g_app->collision_fixer_previews.params.zoom, 0.f, 1.f, "%.2f");
	
		ImGui::EndTable();
	}
	ImGui::PopStyleVar();
	ImGui::PopStyleColor();
	
	static ColladaScene collada_scene;
	static std::string popup_message;
	if (asset && ImGui::Button("Write Collision Mesh")) {
		popup_message = write_instanced_collision(*asset, collada_scene);
		ImGui::OpenPopup("Collision Written");
	}
	
	ImGui::SetNextWindowSize(ImVec2(300, 200));
	if (ImGui::BeginPopupModal("Collision Written")) {
		ImGui::TextWrapped("%s", popup_message.c_str());
		if (ImGui::Button("Okay")) {
			popup_message.clear();
			ImGui::CloseCurrentPopup();
		}
		ImGui::EndPopup();
	}
	
	ImGui::Text("Thread Status: %s", fixer_thread.state_string());
	
	static bool has_run = false;
	static bool waiting_for_completion = false;
	if ((!has_run || params_modified) && fixer_thread.interrupt()) {
		g_app->collision_fixer_previews.collision_mesh = nullptr;
		g_app->collision_fixer_previews.collision_materials = nullptr;
		fixer_thread.start(g_app->game, g_app->game_path, type, o_class, params);
		has_run = true;
		waiting_for_completion = true;
	}
	
	if (waiting_for_completion && !fixer_thread.is_running()) {
		Opt<ColladaScene> out = fixer_thread.get_output();
		if (out.has_value()) {
			collada_scene = std::move(*out);
			verify_fatal(collada_scene.meshes.size() == 1);
			collision_render_mesh = upload_mesh(collada_scene.meshes[0], true);
			collision_materials = upload_collada_materials(collada_scene.materials, {});
			g_app->collision_fixer_previews.collision_mesh = &collision_render_mesh;
			g_app->collision_fixer_previews.collision_materials = &collision_materials;
			waiting_for_completion = false;
		}
	}
}

void shutdown_collision_fixer() {
	fixer_thread.reset();
	preview_class.mesh.reset();
	preview_class.render_mesh.reset();
	preview_class.materials.clear();
	collision_render_mesh = {};
	collision_materials.clear();
	g_app->collision_fixer_previews.collision_mesh = nullptr;
	g_app->collision_fixer_previews.collision_materials = nullptr;
}

void CollisionFixerThread::start(
	Game game, std::string game_bank_path, s32 type, s32 o_class, const ColParams& params)
{
	m_state = STARTING;
	CollisionFixerThread* command = this;
	m_thread = std::thread([command, game, game_bank_path, type, o_class, params]() {
		command->game = game;
		command->game_bank_path = game_bank_path;
		command->type = type;
		command->o_class = o_class;
		command->params = params;
		{
			std::lock_guard<std::mutex> lock(command->m_mutex);
			command->m_state = LOADING_DATA;
		}
		command->run();
	});
}

void CollisionFixerThread::run()
{
	success = false;
	
	auto check_is_still_running = [&]() {
		std::lock_guard<std::mutex> g(m_mutex);
		return m_state == LOADING_DATA || m_state == RECOVERING;
	};
	
	if (!m_loaded) {
		m_bank = &m_forest.mount<LooseAssetBank>(game_bank_path, false);
		BuildAsset& build = m_bank->root()->get_child(game_to_string(game).c_str()).as<BuildAsset>();
		m_levels = load_instance_collision_data(build, check_is_still_running);
		if (!check_is_still_running()) {
			std::lock_guard<std::mutex> g(m_mutex);
			m_state = STOPPED;
			success = false;
			return;
		}
		m_mappings = generate_instance_collision_mappings(m_levels);
		m_loaded = true;
	}
	
	if (!check_is_still_running()) {
		std::lock_guard<std::mutex> g(m_mutex);
		m_state = STOPPED;
		success = false;
		return;
	}
	
	{
		std::lock_guard<std::mutex> g(m_mutex);
		m_state = RECOVERING;
	}
	
	Opt<ColladaScene> s;
	if (type > -1 && o_class > -1) {
		s = build_instanced_collision(type, o_class, params, m_mappings, m_levels, check_is_still_running);
	}
	
	bool result;
	if (s.has_value()) {
		scene = std::move(*s);
		result = true;
	} else {
		scene = {};
		result = false;
	}
	
	{
		std::lock_guard<std::mutex> g(m_mutex);
		m_state = STOPPED;
		success = result;
	}
}

void CollisionFixerThread::reset()
{
	if (m_thread.joinable()) {
		m_thread.join();
	}
	game_bank_path.clear();
	success = false;
	scene = {};
	m_loaded = false;
	m_levels.clear();
	m_mappings.classes[COL_TIE].clear();
	m_mappings.classes[COL_SHRUB].clear();
}

Opt<ColladaScene> CollisionFixerThread::get_output()
{
	std::lock_guard<std::mutex> g(m_mutex);
	if (success) {
		success = false;
		return std::move(scene);
	} else {
		return std::nullopt;
	}
}

bool CollisionFixerThread::interrupt()
{
	{
		std::lock_guard<std::mutex> lock(m_mutex);
		if (m_state == NOT_RUNNING) {
			return true;
		} else if (m_state == LOADING_DATA) {
			return false;
		} else if (m_state != STOPPED) {
			m_state = STOPPING;
		}
	}

	// Wait for the thread to stop processing data.
	for (;;) {
		{
			std::lock_guard<std::mutex> lock(m_mutex);
			if (m_state == STOPPED) {
				m_state = NOT_RUNNING;
				break;
			}
		}
		std::this_thread::sleep_for (std::chrono::milliseconds(5));
	}

	// Wait for the thread to terminate.
	if (m_thread.joinable()) {
		m_thread.join();
	}
	
	return true;
}

bool CollisionFixerThread::is_running()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	return m_state == STARTING || m_state == LOADING_DATA || m_state == RECOVERING;
}

const char* CollisionFixerThread::state_string()
{
	std::lock_guard<std::mutex> lock(m_mutex);
	switch (m_state) {
		case NOT_RUNNING: return "Not Running";
		case STARTING: return "Starting";
		case LOADING_DATA: return "Loading Level Data";
		case RECOVERING: return "Recovering Collision";
		case STOPPING: return "Stopping";
		case STOPPED: return "Stopped";
		default: {}
	}
	return "Error";
}

static std::tuple<s32, s32, Asset*> class_selector()
{
	static AssetSelector tie_selector, shrub_selector;
	tie_selector.required_type_count = 2;
	tie_selector.required_types[0] = TieClassAsset::ASSET_TYPE;
	tie_selector.required_types[1] = ShrubClassAsset::ASSET_TYPE;
	tie_selector.omit_type = LevelAsset::ASSET_TYPE;
	
	s32 type = -1;
	s32 o_class = -1;
	Asset* asset = nullptr;
	if ((asset = asset_selector("##asset", "(select asset)", tie_selector, g_app->asset_forest))) {
		params = {};
		if (asset->logical_type() == TieClassAsset::ASSET_TYPE) {
			TieClassAsset& tie = asset->as<TieClassAsset>();
			Opt<EditorClass> ec = load_tie_editor_class(tie);
			if (ec.has_value() && ec->mesh.has_value() && ec->render_mesh.has_value()) {
				preview_class = std::move(*ec);
				g_app->collision_fixer_previews.mesh = &(*preview_class.render_mesh);
				g_app->collision_fixer_previews.materials = &preview_class.materials;
				generate_bounding_box(*preview_class.mesh);
			} else {
				g_app->collision_fixer_previews.mesh = nullptr;
				g_app->collision_fixer_previews.materials = nullptr;
			}
			type = COL_TIE;
			o_class = tie.id();
		} else {
			ShrubClassAsset& shrub = asset->as<ShrubClassAsset>();
			Opt<EditorClass> ec = load_shrub_editor_class(shrub);
			if (ec.has_value() && ec->mesh.has_value() && ec->render_mesh.has_value()) {
				preview_class = std::move(*ec);
				g_app->collision_fixer_previews.mesh = &(*preview_class.render_mesh);
				g_app->collision_fixer_previews.materials = &preview_class.materials;
				generate_bounding_box(*preview_class.mesh);
			} else {
				g_app->collision_fixer_previews.mesh = nullptr;
				g_app->collision_fixer_previews.materials = nullptr;
			}
			type = COL_SHRUB;
			o_class = shrub.id();
		}
	}
	
	return {type, o_class, asset};
}

static void generate_bounding_box(const Mesh& mesh)
{
	glm::vec3 min;
	glm::vec3 max;
	if (!mesh.vertices.empty()) {
		min = glm::vec3(1000.f, 1000.f, 1000.f);
		max = glm::vec3(-1000.f, -1000.f, -1000.f);
		for (const Vertex& vertex : mesh.vertices) {
			min = glm::min(vertex.pos, min);
			max = glm::max(vertex.pos, max);
		}
	} else {
		min = glm::vec3(-1.f, -1.f, -1.f);
		max = glm::vec3(1.f, 1.f, 1.f);
	}
	params.bounding_box_origin = (min + max) * 0.5f;
	params.bounding_box_size = (max - min) * 2.f;
}

static std::string write_instanced_collision(Asset& asset, const ColladaScene& collision_scene)
{
	std::string message;
	
	AssetType type = asset.logical_type();
	verify_fatal(type == TieClassAsset::ASSET_TYPE || type == ShrubClassAsset::ASSET_TYPE);
	
	if (type == TieClassAsset::ASSET_TYPE) {
		message += write_instanced_collision_for_class_of_type<TieClassAsset>(asset.as<TieClassAsset>(), collision_scene);
	} else if (type == ShrubClassAsset::ASSET_TYPE) {
		message += write_instanced_collision_for_class_of_type<ShrubClassAsset>(asset.as<ShrubClassAsset>(), collision_scene);
	}
	
	return message;
}

template <typename ThisAsset>
static std::string write_instanced_collision_for_class_of_type(
	ThisAsset& asset, const ColladaScene& collision_scene)
{
	std::string message;
	
	CollisionAsset* collision_asset;
	if (&asset.bank() != g_app->mod_bank) {
		AssetLink link = asset.absolute_link();
		Asset* parent = asset.parent();
		verify_fatal(parent);
		std::string path = generate_tie_class_asset_path(asset.id(), *parent);
		AssetFile& new_file = g_app->mod_bank->asset_file(path);
		ThisAsset& new_asset = new_file.asset_from_link(ThisAsset::ASSET_TYPE, link).template as<ThisAsset>();
		collision_asset = &new_asset.static_collision();
	} else {
		collision_asset = &asset.static_collision();
	}
	
	MeshAsset& mesh_asset = collision_asset->mesh();
	std::vector<u8> collada = write_collada(collision_scene);
	mesh_asset.set_name("collision");
	FileReference src = mesh_asset.file().write_text_file("recovered_collision.dae", (char*) collada.data());
	mesh_asset.set_src(src);
	message += stringf("Written file: %s\n", src.path.string().c_str());
	
	CollectionAsset& materials = collision_asset->materials();
	for (const ColladaMaterial& material : collision_scene.materials) {
		CollisionMaterialAsset& asset = materials.child<CollisionMaterialAsset>(material.name.c_str());
		asset.set_name(material.name);
		asset.set_id(material.collision_id);
	}
	
	collision_asset->file().write();
	message += stringf("Written file: %s\n", collision_asset->file().path().c_str());
	
	return message;
}
