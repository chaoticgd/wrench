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

#include "editor.h"

#include <utility>

void BaseEditor::undo() {
	verify(m_command_past_last >= 1, "Nothing to undo.");
	UndoRedoCommand& cmd = m_commands[--m_command_past_last];
	cmd.undo(*this, cmd.user_data);
}

void BaseEditor::redo() {
	verify(m_command_past_last < m_commands.size(), "Nothing to redo.");
	UndoRedoCommand& cmd = m_commands[m_command_past_last++];
	cmd.apply(*this, cmd.user_data);
}

// Gross move semantics boilerplate -- don't look!

BaseEditor::UndoRedoCommand::UndoRedoCommand() {}

BaseEditor::UndoRedoCommand::UndoRedoCommand(UndoRedoCommand&& rhs) noexcept
	: user_data(std::exchange(rhs.user_data, nullptr))
	, apply(std::move(rhs.apply))
	, undo(std::move(rhs.undo))
	, cleanup(std::exchange(rhs.cleanup, nullptr)) {}

BaseEditor::UndoRedoCommand::~UndoRedoCommand() {
	if (cleanup && user_data) {
		cleanup(user_data);
	}
}

BaseEditor::UndoRedoCommand& BaseEditor::UndoRedoCommand::operator=(UndoRedoCommand& rhs) noexcept {
	if (cleanup && user_data) {
		cleanup(user_data);
	}
	user_data = std::exchange(rhs.user_data, nullptr);
	apply = std::move(rhs.apply);
	undo = std::move(rhs.undo);
	cleanup = std::exchange(rhs.cleanup, nullptr);
	return *this;
}
