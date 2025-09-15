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

#include "remote.h"

#include <map>

#define NOMINMAX
#include <pine.h>

namespace remote {

static void update_range(u32 begin_address, u32 end_address, std::vector<u32>& reply_addresses);
static const char* ipc_status_to_string(PINE::PCSX2::IPCStatus status);

static const u32 EE_MEMORY_SIZE = 32 * 1024 * 1024;

static std::unique_ptr<PINE::PCSX2> s_ipc;
static std::map<std::string, CppType> s_types;
static std::multimap<u32, Object*> s_objects;
static std::array<u8, EE_MEMORY_SIZE> s_ee_memory;
static std::array<std::chrono::milliseconds, EE_MEMORY_SIZE / 8> s_last_update_time;

void connect()
{
	try {
		s_ipc = std::make_unique<PINE::PCSX2>();
		
		// Throw if we're not connected.
		s_ipc->Status<false>();
	} catch (PINE::PCSX2::IPCStatus status) {
		verify_not_reached("Failed to connect (%s).", ipc_status_to_string(status));
	}
	
	memset(s_ee_memory.data(), 0, s_ee_memory.size());
	memset(s_last_update_time.data(), 0, s_last_update_time.size());
}

void update()
{
	// Request the contents of all the memory we're interested in.
	s_ipc->InitializeBatch();
	
	static std::vector<u32> reply_addresses;
	reply_addresses.clear();
	
	try {
		u32 begin_address = 0;
		u32 end_address = 0;
		for (const auto& [address, object] : s_objects) {
			if (address > end_address) {
				update_range(begin_address, end_address, reply_addresses);
				begin_address = address;
				end_address = address + object->size();
			} else if (address + object->size() > end_address) {
				end_address = address + object->size();
			}
		}
		update_range(begin_address, end_address, reply_addresses);
	} catch (PINE::PCSX2::IPCStatus status) {
		s_ipc->FinalizeBatch();
		verify_not_reached("Failed to issue read command (%s).", ipc_status_to_string(status));
	}
	
	PINE::PCSX2::BatchCommand batch = s_ipc->FinalizeBatch();
	
	std::chrono::milliseconds current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch());
	
	try {
		s_ipc->SendCommand(batch);
	} catch (PINE::PCSX2::IPCStatus status) {
		verify_not_reached("Failed to send command list in update (%s).", ipc_status_to_string(status));
	}
	
	// Read the contents of said memory back.
	for (size_t i = 0; i < reply_addresses.size(); i++) {
		u64 address = reply_addresses[i];
		u64 value = s_ipc->GetReply<PINE::PCSX2::MsgRead64>(batch, (int) i);
		
		*(u64*) &s_ee_memory[address] = value;
		s_last_update_time[address / 8] = current_time;
	}
}

static void update_range(u32 begin_address, u32 end_address, std::vector<u32>& reply_addresses)
{
	end_address = std::min(end_address, EE_MEMORY_SIZE);
	
	for (u32 address = begin_address & ~7; address < end_address; address += 8) {
		s_ipc->Read<u64, true>(address);
		reply_addresses.emplace_back(address);
	}
}

void disconnect()
{
	verify(s_objects.empty(), "Failed to disconnect: Remote objects still exist!");
	
	s_ipc.reset(nullptr);
}

const char* game_id()
{
	static const char* game_id = nullptr;
	if (game_id != nullptr)
		delete[] game_id;
	game_id = s_ipc->GetGameID();
	return game_id;
}

static const char* ipc_status_to_string(PINE::PCSX2::IPCStatus status)
{
	switch (status)
	{
		case PINE::PCSX2::Success: return "Success";
		case PINE::PCSX2::Fail: return "Fail";
		case PINE::PCSX2::OutOfMemory: return "OutOfMemory";
		case PINE::PCSX2::NoConnection: return "NoConnection";
		case PINE::PCSX2::Unimplemented: return "Unimplemented";
		case PINE::PCSX2::Unknown: return "Unknown";
	}
	return "Other";
}

// *****************************************************************************

Object::Object(u32 address, u32 size)
	: m_address(address)
	, m_size(size)
{
	verify(s_ipc, "Remote object created while disconnected!");
	
	register_self();
}

Object::Object(const Object& rhs)
	: m_address(rhs.m_address)
	, m_size(rhs.m_size)
{
	register_self();
}

Object::Object(Object&& rhs)
	: m_address(rhs.m_address)
	, m_size(rhs.m_size)
{
	rhs.deregister_self();
	register_self();
}

Object::~Object()
{
	deregister_self();
}

Object& Object::operator=(const Object& rhs)
{
	deregister_self();
	m_address = rhs.m_address;
	m_size = rhs.m_size;
	register_self();
	return *this;
}

Object& Object::operator=(Object&& rhs)
{
	rhs.deregister_self();
	deregister_self();
	m_address = rhs.m_address;
	m_size = rhs.m_size;
	register_self();
	return *this;
}

bool Object::is_up_to_date(std::chrono::milliseconds max_latency, std::chrono::milliseconds current_time) const
{
	u32 index = m_address / 8;
	if (index >= s_last_update_time.size())
		return false;
	
	return (current_time - s_last_update_time[index]) < max_latency;
}

u32 Object::address() const
{
	return m_address;
}

u32 Object::size() const
{
	return m_size;
}

bool Object::read(u32 offset, u8* data, u32 size)
{
	u32 address = m_address + offset;
	if (address > EE_MEMORY_SIZE || EE_MEMORY_SIZE - address < size) {
		memset(data, 0, size);
		return false;
	}
	
	if (size > 0 && s_last_update_time[address / 8].count() == 0) {
		memset(data, 0, size);
		return false;
	}
	
	memcpy(data, &s_ee_memory[address], size);
	
	return true;
}

bool Object::write(u32 offset, const u8* data, u32 size)
{
	u32 address = m_address + offset;
	if (address > EE_MEMORY_SIZE || EE_MEMORY_SIZE - address < size) {
		return false;
	}
	
	// Write to the cache.
	memcpy(&s_ee_memory[address], data, size);
	
	// Write to the emulator memory.
	s_ipc->InitializeBatch();
	
	std::chrono::milliseconds current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch());
	
	try {
		for (u32 i = 0; i < size; i++) {
			s_ipc->Write<u8, true>(address + i, s_ee_memory[address + i]);
			s_last_update_time[(address + i) / 8] = current_time;
		}
	} catch (PINE::PCSX2::IPCStatus status) {
		s_ipc->FinalizeBatch();
		verify_not_reached("Failed to issue write command (%s).",
			ipc_status_to_string(status));
	}
	
	try {
		s_ipc->SendCommand(s_ipc->FinalizeBatch());
	} catch (PINE::PCSX2::IPCStatus status) {
		verify_not_reached("Failed to send command list for in Object::write (%s).",
			ipc_status_to_string(status));
	}
	
	return true;
}

void Object::sync()
{
	s_ipc->InitializeBatch();
	
	try {
		for (u32 i = 0; i < m_size && (m_address + i) < EE_MEMORY_SIZE; i++) {
			s_ipc->Read<u8, true>(m_address + i);
		}
	} catch (PINE::PCSX2::IPCStatus status) {
		s_ipc->FinalizeBatch();
		verify_not_reached("Failed to issue read command (%s).",
			ipc_status_to_string(status));
	}
	
	PINE::PCSX2::BatchCommand batch = s_ipc->FinalizeBatch();
	
	std::chrono::milliseconds current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
		std::chrono::steady_clock::now().time_since_epoch());
	
	try {
		s_ipc->SendCommand(batch);
	} catch (PINE::PCSX2::IPCStatus status) {
		verify_not_reached("Failed to send command list in Object::sync (%s).",
			ipc_status_to_string(status));
	}
	
	for (u32 i = 0; i < m_size && (m_address + i) < EE_MEMORY_SIZE; i++) {
		u8 value = s_ipc->GetReply<PINE::PCSX2::MsgRead8>(batch, i);
		s_ee_memory[m_address + i] = value;
		s_last_update_time[(m_address + i) / 8] = current_time;
	}
}

void Object::register_self()
{
	s_objects.emplace(m_address, this);
}

void Object::deregister_self()
{
	auto [begin, end] = s_objects.equal_range(m_address);
	for (auto iterator = begin; iterator != end; iterator++)
	{
		if (iterator->second == this)
		{
			s_objects.erase(iterator);
			break;
		}
	}
}

}

#define GENERATED_TRAINER_TYPES_IMPLEMENTATION
#include "_generated_trainer_types.inl"
#undef GENERATED_TRAINER_TYPES_IMPLEMENTATION
