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

#include <chrono>

#include <toolwads/wads.h>
#include <gui/gui.h>
#include <gui/config.h>
#include <gui/commands.h>
#include <imgui/backends/imgui_impl_glfw.h>
#include <editor/fs_includes.h>
#include <editor/app.h>
#include <editor/gui/editor_gui.h>
#include <editor/renderer.h>

static void run_wrench(GLFWwindow* window, const WadPaths& wad_paths, const std::string& game_path, const std::string& mod_path);
static void update(f32 delta_time);
static void update_camera(app* a);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

int main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "usage: %s <game path> <mod path>\n", argv[0]);
		return 1;
	}
	
	WadPaths wads = find_wads(argv[0]);
	g_guiwad.open(wads.gui);
	g_editorwad.open(wads.editor);
	
	gui::setup_bin_paths(argv[0]);
	
	g_config.read();
	
	std::string game_path = argv[1];
	std::string mod_path = argv[2];
	
	gui::GlfwCallbacks callbacks;
	callbacks.key_callback = key_callback;
	
	GLFWwindow* window = gui::startup("Wrench Editor", 1280, 720, true, &callbacks);
	run_wrench(window, wads, game_path, mod_path);
	gui::shutdown(window);
}


static void run_wrench(
	GLFWwindow* window,
	const WadPaths& wad_paths,
	const std::string& game_path,
	const std::string& mod_path)
{
	app a;
	g_app = &a;
	
	a.glfw_window = window;
	
	glfwSetWindowUserPointer(window, g_app);
	glfwSetKeyCallback(window, key_callback);
	
	a.game_path = game_path;
	a.overlay_path = wad_paths.overlay;
	a.mod_path = mod_path;
	
	// Load the underlay, and mark all underlay assets as weakly deleted so they
	// don't show up if the asset isn't actually present.
	a.underlay_bank = &a.asset_forest.mount<LooseAssetBank>(wad_paths.underlay.c_str(), false);
	a.asset_forest.any_root()->for_each_logical_descendant([&](Asset& asset) {
		// If the asset has strongly_deleted set to false, interpret that to
		// mean the asset should shouldn't be weakly deleted.
		if ((asset.flags & ASSET_HAS_STRONGLY_DELETED_FLAG) == 0 || (asset.flags & ASSET_IS_STRONGLY_DELETED) != 0) {
			asset.flags |= ASSET_IS_WEAKLY_DELETED;
		}
	});
	
	a.game_bank = &a.asset_forest.mount<LooseAssetBank>(game_path, false);
	a.overlay_bank = &a.asset_forest.mount<LooseAssetBank>(wad_paths.overlay.c_str(), false);
	a.mod_bank = &a.asset_forest.mount<LooseAssetBank>(mod_path, true);
	
	a.asset_forest.read_source_files(a.game_bank->game_info.game.game);
	
	verify(a.game_bank->game_info.type == AssetBankType::GAME,
		"The asset bank specified for the game is not a game.");
	a.game = a.game_bank->game_info.game.game;
		
	gui::load_font(wadinfo.gui.fonts[0], 18, 1.2f);
	
	init_renderer();
	
	g_tools[g_active_tool]->funcs.activate();
	
	while (!glfwWindowShouldClose(a.glfw_window)) {
		gui::run_frame(window, update);
	}
	
	a.last_frame = true;
	gui::run_frame(window, update);
	
	g_tools[g_active_tool]->funcs.deactivate();
	
	shutdown_renderer();
}

static void update(f32 delta_time)
{
	g_app->delta_time = delta_time;
	update_camera(g_app);
	editor_gui();
}

static void update_camera(app* a)
{
	// Rotation
	double xpos, ypos;
	glfwGetCursorPos(a->glfw_window, &xpos, &ypos);
	
	glm::vec2 mouse_cur = glm::vec2(xpos, ypos);
	glm::vec2 mouse_diff = mouse_cur - a->mouse_last;
	a->mouse_last = mouse_cur;
	
	if (!a->render_settings.camera_control) {
		return;
	}
	
	static const auto constrain = [](float* ptr, float min, float max, bool should_flip) {
		if (*ptr < min)
			*ptr = (should_flip) ? max : min;
		if (*ptr > max)
			*ptr = (should_flip) ? min : max;
	};
	
	static const float min_pitch = glm::radians(-89.f), max_pitch = glm::radians(89.f);
	static const float min_yaw = glm::radians(-180.f),  max_yaw = glm::radians(180.f);
	
	a->render_settings.camera_rotation.y += mouse_diff.x * 0.0005;
	a->render_settings.camera_rotation.x -= mouse_diff.y * 0.0005;
	
	constrain(&a->render_settings.camera_rotation.y, min_yaw, max_yaw, true);
	constrain(&a->render_settings.camera_rotation.x, min_pitch, max_pitch, false);
	
	// Position
	float dist = 2;
	float dx = std::sin(a->render_settings.camera_rotation.y) * dist;
	float dz = std::cos(a->render_settings.camera_rotation.y) * dist;
	
	auto is_down = [&](int key) {
		return glfwGetKey(a->glfw_window, key);
	};
	
	glm::vec3 movement(0, 0, 0);
	
	static constexpr float speed = 30.f;
	
	if (is_down(GLFW_KEY_W)) {
		movement.x -= dz * a->delta_time * speed;
		movement.y += dx * a->delta_time * speed;
	}
	if (is_down(GLFW_KEY_S)) {
		movement.x += dz * a->delta_time * speed;
		movement.y -= dx * a->delta_time * speed;
	}
	if (is_down(GLFW_KEY_A)) {
		movement.x -= dx * a->delta_time * speed;
		movement.y -= dz * a->delta_time * speed;
	}
	if (is_down(GLFW_KEY_D)) {
		movement.x += dx * a->delta_time * speed;
		movement.y += dz * a->delta_time * speed;
	}
	if (is_down(GLFW_KEY_SPACE)) {
		movement.z += dist * a->delta_time * speed;
	}
	if (is_down(GLFW_KEY_LEFT_SHIFT)) {
		movement.z -= dist * a->delta_time * speed;
	}
	a->render_settings.camera_position += movement;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	app* a = static_cast<app*>(glfwGetWindowUserPointer(window));
	
	if (action == GLFW_PRESS && key == GLFW_KEY_Z) {
		a->render_settings.camera_control = !a->render_settings.camera_control;
		glfwSetInputMode(window, GLFW_CURSOR,
			a->render_settings.camera_control ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
	}
	
	if (!a->render_settings.camera_control) {
		ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
	}
}
