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

#ifndef EDITOR_UNDO_REDO_H
#define EDITOR_UNDO_REDO_H

#include <functional>

#include <core/util.h>
#include <core/filesystem.h>

class BaseEditor
{
public:
	virtual ~BaseEditor() {}

	void undo();
	void redo();
	
	virtual std::string save() = 0;
	
protected:
	struct UndoRedoCommand
	{
		UndoRedoCommand();
		UndoRedoCommand(const UndoRedoCommand&) = delete;
		UndoRedoCommand(UndoRedoCommand&& rhs) noexcept;
		~UndoRedoCommand();
		UndoRedoCommand& operator=(const UndoRedoCommand&) = delete;
		UndoRedoCommand& operator=(UndoRedoCommand& rhs) noexcept;
		
		void* user_data = nullptr;
		std::function<void(BaseEditor& editor, void* user_data)> apply;
		std::function<void(BaseEditor& editor, void* user_data)> undo;
		void (*cleanup)(void* user_data) = nullptr;
	};

	std::vector<UndoRedoCommand> m_commands;
	size_t m_command_past_last = 0;
	bool m_pushing_command = false;
};

template <typename ThisEditor>
class Editor : public BaseEditor
{
public:
	template <typename UserData>
	void push_command(
		UserData data,
		void (*apply)(ThisEditor& editor, UserData& data),
		void (*undo)(ThisEditor& editor, UserData& data))
	{
		verify(!m_pushing_command, "Recursively entered Editor::push_command.");
		m_pushing_command = true;
		defer([&]() { m_pushing_command = false; });
		
		UndoRedoCommand cmd;
		cmd.user_data = new UserData(std::move(data));
		cmd.apply = [apply](BaseEditor& editor, void* user_data) {
			apply(static_cast<ThisEditor&>(editor), *(UserData*) user_data);
		};
		cmd.undo = [undo](BaseEditor& editor, void* user_data) {
			undo(static_cast<ThisEditor&>(editor), *(UserData*) user_data);
		};
		cmd.cleanup = [](void* user_data) {
			delete (UserData*) user_data;
		};
		
		cmd.apply(*this, cmd.user_data);
		
		m_commands.resize(m_command_past_last);
		m_commands.emplace_back(std::move(cmd));
		m_command_past_last = m_commands.size();
	}
};

#endif
