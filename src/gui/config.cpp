/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

#include "config.h"

#include <core/stream.h>
#include <core/filesystem.h>

void gui::PathConfig::read(const WtfNode* node)
{
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "base_folder", WTF_STRING)) {
		base_folder = attrib->string.begin;
	} else {
#ifdef _WIN32
		base_folder = ".";
#else
		char* home = getenv("HOME");
		verify(home, "HOME enviroment variable not set!");
		base_folder = std::string(home) + "/wrench";
#endif
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "mods_folders", WTF_ARRAY)) {
		for (const WtfAttribute* element = attrib->first_array_element; element != nullptr; element = element->next) {
			if (element->type == WTF_STRING) {
				mods_folders.emplace_back(element->string.begin);
			}
		}
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "games_folder", WTF_STRING)) {
		games_folder = attrib->string.begin;
	} else {
		if (base_folder != ".") {
			games_folder = base_folder + "/games";
		} else {
			games_folder = "games";
		}
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "builds_folder", WTF_STRING)) {
		builds_folder = attrib->string.begin;
	} else {
		if (base_folder != ".") {
			builds_folder = base_folder + "/builds";
		} else {
			builds_folder = "builds";
		}
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "cache_folder", WTF_STRING)) {
		cache_folder = attrib->string.begin;
	} else {
		if (base_folder != ".") {
			cache_folder = base_folder + "/cache";
		} else {
			cache_folder = "cache";
		}
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "emulator_path", WTF_STRING)) {
		emulator_path = attrib->string.begin;
	}
}

void gui::PathConfig::write(WtfWriter* ctx) const
{
	wtf_begin_node(ctx, nullptr, "paths");
	
	wtf_write_string_attribute(ctx, "base_folder", base_folder.c_str());
	wtf_begin_attribute(ctx, "mods_folders");
	wtf_begin_array(ctx);
	for (const std::string& mods_folder : mods_folders) {
		wtf_write_string(ctx, mods_folder.c_str());
	}
	wtf_end_array(ctx);
	wtf_end_attribute(ctx);
	wtf_write_string_attribute(ctx, "games_folder", games_folder.c_str());
	wtf_write_string_attribute(ctx, "builds_folder", builds_folder.c_str());
	wtf_write_string_attribute(ctx, "cache_folder", cache_folder.c_str());
	wtf_write_string_attribute(ctx, "emulator_path", emulator_path.c_str());
	
	wtf_end_node(ctx);
}

void gui::UiConfig::read(const WtfNode* node)
{
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "custom_scale", WTF_BOOLEAN)) {
		custom_scale = attrib->boolean;
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "scale", WTF_NUMBER)) {
		scale = attrib->number.f;
	}
	if (const WtfAttribute* attrib = wtf_attribute_of_type(node, "developer", WTF_BOOLEAN)) {
		developer = attrib->boolean;
	}
}

void gui::UiConfig::write(WtfWriter* ctx) const
{
	wtf_begin_node(ctx, nullptr, "ui");
	
	wtf_write_boolean_attribute(ctx, "custom_scale", custom_scale);
	wtf_write_float_attribute(ctx, "scale", scale);
	wtf_write_boolean_attribute(ctx, "developer", developer);
	
	wtf_end_node(ctx);
}

void gui::Config::read()
{
	FileInputStream stream;
	if (stream.open(get_config_file_path())) {
		std::vector<u8> text = stream.read_multiple<u8>(stream.size());
		strip_carriage_returns(text);
		text.push_back(0);
		
		char* error_dest = nullptr;
		WtfNode* root = wtf_parse((char*) text.data(), &error_dest);
		if (error_dest) {
			fprintf(stderr, "Failed to read config: %s\n", error_dest);
			return;
		}
		
		*this = {};
		paths.read(wtf_child(root, nullptr, "paths"));
		ui.read(wtf_child(root, nullptr, "ui"));
	}
}

void gui::Config::write() const
{
	FileOutputStream stream;
	if (stream.open(get_config_file_path())) {
		std::string text;
		WtfWriter* ctx = wtf_begin_file(text);
		paths.write(ctx);
		ui.write(ctx);
		wtf_end_file(ctx);
		stream.write_n((u8*) text.data(), text.size());
	}
}

void gui::Config::set_to_defaults()
{
	*this = {};
	WtfNode dummy = {};
	paths.read(&dummy);
	if (paths.base_folder == ".") {
		paths.mods_folders.emplace_back("mods");
	} else {
		paths.mods_folders.emplace_back(paths.base_folder + "/mods");
	}
	ui.read(&dummy);
}

std::string gui::get_config_file_path()
{
#ifdef _WIN32
	return "wrench.cfg";
#else
	const char* home = getenv("HOME");
	verify(home, "HOME enviroment variable not set!");
	return std::string(home) + "/.config/wrench.cfg";
#endif
}

bool gui::config_file_exists() {
	return fs::exists(get_config_file_path());
}

gui::Config g_config = {};
