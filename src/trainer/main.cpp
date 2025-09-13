/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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
#include <thread>

#include <gui/gui.h>
#include <toolwads/wads.h>

#include <trainer/remote.h>
#include <trainer/trainer.h>

static void wait_one_second(GLFWwindow* window);

static std::unique_ptr<Trainer> s_trainer;
static FileInputStream s_trainerwad;
static bool s_should_disconnect = false;
static std::string s_last_error;

int main(int argc, char** argv)
{
	WadPaths wads = find_wads(argv[0]);
	verify(g_guiwad.open(wads.gui), "Failed to open gui wad.");
	verify(s_trainerwad.open(wads.trainer), "Failed to open memcard wad.");
	
	GLFWwindow* window = gui::startup("Wrench Trainer", 1280, 720);
	gui::load_font(wadinfo.gui.fonts[0], 18, 1.2f);
	
	while (!glfwWindowShouldClose(window)) {
		// Connect to the emulator.
		try {
			remote::connect();
		} catch (const RuntimeError& error) {
			s_last_error = error.message;
			wait_one_second(window);
			continue;
		}
		
		// We can start creating remote objects now.
		try {
			s_trainer = std::make_unique<Trainer>();
		} catch (const RuntimeError& error) {
			fprintf(stderr, "error: %s\n", error.message.c_str());
			s_last_error = error.message;
			wait_one_second(window);
			s_should_disconnect = true;
		}
		
		// Main loop.
		while (!s_should_disconnect && !glfwWindowShouldClose(window)) {
			try {
				remote::update();
			} catch (const RuntimeError& error) {
				fprintf(stderr, "error: %s\n", error.message.c_str());
				s_last_error = error.message;
				break;
			}
			gui::run_frame(window, [](f32 delta_time) {
				s_should_disconnect = s_trainer->update(delta_time);
			});
		}
	
		// We must destroy all remote objects before disconnecting.
		s_trainer.reset(nullptr);
		s_should_disconnect = false;
		
		// Disconnect from the emulator.
		try {
			remote::disconnect();
		} catch (const RuntimeError& error) {
			fprintf(stderr, "error: %s\n", error.message.c_str());
			s_last_error = error.message;
		}
	}
	
	gui::shutdown(window);
}

static void wait_one_second(GLFWwindow* window)
{
	for (int i = 0; i < 10; i++)
	{
		gui::run_frame(window, [](f32 delta_time) {
			if (ImGui::BeginMainMenuBar()) {
				ImGui::Text("ERROR: %s", s_last_error.c_str());
				ImGui::EndMainMenuBar();
			}
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		});
	}
}
