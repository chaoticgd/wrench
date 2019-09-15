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

#ifndef LEVEL_H
#define LEVEL_H

#include <map>
#include <set>
#include <memory>
#include <vector>
#include <glm/glm.hpp>

#include "model.h"
#include "command.h"
#include "texture.h"
#include "inspectable.h"
#include "imgui_includes.h"

# /*
#	Virtual base classes for levels and game objects (shrubs, mobies, ...).
#	
#	Mobies represent moving objects in the game world.
#	Shrubs are decorative objects e.g. plants or small rocks.
#	
# */

class game_object;
class point_object;
class tie;
class shrub;
class spline;
class moby;

class base_level {
public:
	base_level();
	virtual ~base_level() = default;

	// Game data
	virtual texture_provider* get_texture_provider() = 0;
	const texture_provider* get_texture_provider() const;

	virtual void for_each_game_object(std::function<void(game_object*)> callback) = 0;

	virtual std::map<std::string, std::map<uint32_t, std::string>> game_strings() = 0;

	// Selection
	std::vector<std::size_t> selection;
	bool is_selected(const game_object* obj) const;
	
	void inspect(inspector_callbacks* cb);
};

class game_object : public inspectable {
public:
	void inspect(inspector_callbacks* cb);

	virtual std::size_t base() const = 0;
	std::string base_string() const;
};

class point_object : public game_object {
public:
	void inspect(inspector_callbacks* cb);

	virtual glm::vec3 position() const = 0;
	virtual void set_position(glm::vec3 rotation_) = 0;

	virtual glm::vec3 rotation() const = 0;
	virtual void set_rotation(glm::vec3 rotation_) = 0;

	virtual std::string label() const = 0;
};

class base_tie : public point_object {
public:
	void inspect(inspector_callbacks* cb);
	
	virtual const model& object_model() const = 0;
};

class base_shrub : public point_object {
public:
	void inspect(inspector_callbacks* cb);
};

class base_spline : public game_object {
public:
	void inspect(inspector_callbacks* cb);
	
	virtual std::vector<glm::vec3> points() const = 0;
};

class base_moby : public point_object {
public:
	virtual ~base_moby() = default;

	void inspect(inspector_callbacks* cb);

	virtual int32_t uid() const = 0;
	virtual void set_uid(int32_t uid_) = 0;

	virtual uint16_t class_num() const = 0;
	virtual void set_class_num(uint16_t class_num_) = 0;

	virtual std::string class_name() const = 0;

	virtual const model& object_model() const = 0;
};

#endif
