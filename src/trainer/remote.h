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

#ifndef TRAINER_REMOTE_H
#define TRAINER_REMOTE_H

#include <chrono>

#include <core/util.h>
#include <cppparser/cpp_type.h>

namespace remote {

void connect();
void update();
void disconnect();

const char* game_id();

class Object
{
public:
	Object(u32 address, u32 size);
	Object(const Object& rhs);
	Object(Object&& rhs);
	~Object();
	
	Object& operator=(const Object& rhs);
	Object& operator=(Object&& rhs);
	
	bool is_up_to_date(std::chrono::milliseconds max_latency, std::chrono::milliseconds current_time) const;
	
	u32 address() const;
	u32 size() const;
	
	bool read(u32 offset, u8* data, u32 size);
	bool write(u32 offset, const u8* data, u32 size);
	
	void sync();
	
private:
	void register_self();
	void deregister_self();
	
	u32 m_address = 0;
	u32 m_size = 0;
};

}

#define GENERATED_TRAINER_TYPES_HEADER
#include "_generated_trainer_types.inl"
#undef GENERATED_TRAINER_TYPES_HEADER

#endif
