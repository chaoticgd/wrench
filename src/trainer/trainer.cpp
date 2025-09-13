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

#include "trainer.h"

#include <core/release.h>
#include <gui/gui.h>

Trainer::Trainer()
	: m_game_id(remote::game_id())
	, m_game(identify_release_fuzzy(m_game_id).game)
	, m_level(0x0015ed84, 4)
{
	m_level.sync();
	m_level.read(0, (u8*) &m_level_backup, 4);
}

bool Trainer::update(f32 delta_time)
{
	s32 level;
	m_level.sync();
	m_level.read(0, (u8*) &level, 4);
	
	const char* game_id = remote::game_id();
	if (game_id != m_game_id || level != m_level_backup) {
		// We need to reset.
		return false;
	}
	
	if (ImGui::BeginMainMenuBar()) {
		ImGui::Text("SERIAL: %s", game_id);
		ImGui::SameLine();
		ImGui::Text("GAME: %s", game_to_string(m_game).c_str());
		ImGui::SameLine();
		ImGui::Text("LEVEL: %d", m_level_backup);
		ImGui::EndMainMenuBar();
	}
	
	return true;
}
