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

#include "instances.h"

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>

void Instances::clear_selection() {
	this->for_each([&](Instance& inst) {
		inst.selected = false;
	});
}

std::vector<InstanceId> Instances::selected_instances() const {
	std::vector<InstanceId> ids;
	this->for_each([&](const Instance& inst) {
		if(inst.selected) {
			ids.push_back(inst.id());
		}
	});
	return ids;
}

struct InstanceReadWriteFuncs {
	InstanceType type;
	void (*read)(Instances& dest, const WtfNode* src); 
	void (*write)(WtfWriter* dest, const Instances& src);
};

#define GENERATED_INSTANCE_READ_WRITE_TABLE
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_READ_WRITE_TABLE

Instances read_instances(std::string& src) {
	char* error = nullptr;
	WtfNode* root = wtf_parse((char*) src.c_str(), &error);
	verify(!error && root, "Failed to parse instances file. %s", error);
	defer([&]() { wtf_free(root); });
	
	Instances dest;
	
	const WtfNode* level_settings_node = wtf_child(root, nullptr, "level_settings");
	if(level_settings_node) {
		dest.level_settings = read_level_settings(level_settings_node);
	}
	
	for(WtfNode* node = root->first_child; node != nullptr; node = node->next_sibling) {
		for(const InstanceReadWriteFuncs& funcs : read_write_funcs) {
			if(strcmp(instance_type_to_string(funcs.type), node->type_name) == 0) {
				funcs.read(dest, node);
			}
		}
	}
	
	return dest;
}

std::string write_instances(const Instances& src) {
	std::string dest;
	WtfWriter* ctx = wtf_begin_file(dest);
	defer([&]() { wtf_end_file(ctx); });
	
	wtf_begin_node(ctx, nullptr, "level_settings");
	write_level_settings(ctx, src.level_settings);
	wtf_end_node(ctx);
	
	for(const InstanceReadWriteFuncs& funcs : read_write_funcs) {
		funcs.write(ctx, src);
	}
	
	return dest;
}
