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

Instance* Instances::from_id(InstanceId id)
{
	switch (id.type) {
		#define DEF_INSTANCE(inst_type, inst_type_uppercase, inst_variable, link_type) \
			case INST_##inst_type_uppercase: return inst_variable.from_id(id.value);
		#define GENERATED_INSTANCE_MACRO_CALLS
		#include "_generated_instance_types.inl"
		#undef GENERATED_INSTANCE_MACRO_CALLS
		#undef DEF_INSTANCE
		default: {}
	}
	return nullptr;
}

void Instances::clear_selection()
{
	this->for_each([&](Instance& inst) {
		inst.selected = false;
	});
}

std::vector<InstanceId> Instances::selected_instances() const
{
	std::vector<InstanceId> ids;
	this->for_each([&](const Instance& inst) {
		if (inst.selected) {
			ids.push_back(inst.id());
		}
	});
	return ids;
}

struct InstanceReadWriteFuncs
{
	InstanceType type;
	void (*read)(Instances& dest, const WtfNode* src); 
	void (*write)(WtfWriter* dest, const Instances& src);
};

#define GENERATED_INSTANCE_READ_WRITE_TABLE
#include "_generated_instance_types.inl"
#undef GENERATED_INSTANCE_READ_WRITE_TABLE

Instances read_instances(std::string& src)
{
	char* error = nullptr;
	WtfNode* root = wtf_parse(src.data(), &error);
	verify(!error && root, "Failed to parse instances file. %s", error);
	defer([&]() { wtf_free(root); });
	
	Instances dest;
	
	const WtfNode* level_settings_node = wtf_child(root, nullptr, "level_settings");
	if (level_settings_node) {
		dest.level_settings = read_level_settings(level_settings_node);
	}
	
	for (WtfNode* node = root->first_child; node != nullptr; node = node->next_sibling) {
		for (const InstanceReadWriteFuncs& funcs : read_write_funcs) {
			if (strcmp(instance_type_to_string(funcs.type), node->type_name) == 0) {
				funcs.read(dest, node);
			}
		}
	}
	
	const WtfAttribute* moby_classes_attrib = wtf_attribute_of_type(root, "moby_classes", WTF_ARRAY);
	for (WtfAttribute* o_class = moby_classes_attrib->first_array_element; o_class != nullptr; o_class = o_class->next) {
		verify(o_class->type == WTF_NUMBER, "Bad moby class number.");
		dest.moby_classes.emplace_back(o_class->number.i);
	}
	
	const WtfAttribute* spawnable_moby_count_attrib = wtf_attribute_of_type(root, "spawnable_moby_count", WTF_NUMBER);
	verify(spawnable_moby_count_attrib, "Missing 'spawnable_moby_count' field.");
	dest.spawnable_moby_count = spawnable_moby_count_attrib->number.i;
	
	return dest;
}

std::string write_instances(const Instances& src, const char* application_name, const char* application_version)
{
	std::string dest;
	WtfWriter* ctx = wtf_begin_file(dest);
	defer([&]() { wtf_end_file(ctx); });
	
	wtf_begin_node(ctx, nullptr, "version_info");
	wtf_write_string_attribute(ctx, "application_name", application_name);
	wtf_write_string_attribute(ctx, "application_version", application_version);
	wtf_write_integer_attribute(ctx, "format_version", INSTANCE_FORMAT_VERSION);
	wtf_end_node(ctx);
	
	wtf_begin_node(ctx, nullptr, "level_settings");
	write_level_settings(ctx, src.level_settings);
	wtf_end_node(ctx);
	
	for (const InstanceReadWriteFuncs& funcs : read_write_funcs) {
		funcs.write(ctx, src);
	}
	
	wtf_begin_attribute(ctx, "moby_classes");
	wtf_begin_array(ctx);
	for (s32 o_class : src.moby_classes) {
		wtf_write_integer(ctx, o_class);
	}
	wtf_end_array(ctx);
	wtf_end_attribute(ctx);
	
	wtf_write_integer_attribute(ctx, "spawnable_moby_count", src.spawnable_moby_count);
	
	return dest;
}
