/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2021 chaoticgd

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

#include "level.h"

// Given two disjoint gameplay structs first and second, this will merge all
// members into the first.
struct GameplayMerger {
	Gameplay* first;
	Gameplay* second;
	template <typename T>
	void field(const char*, T&) {}
	template <typename T>
	bool field_ptr(const char* name, T ptr) {
		auto& thing = first->*ptr;
		auto& other = second->*ptr;
		assert(!(thing.has_value() && other.has_value()));
		if(other.has_value()) {
			thing.swap(other);
		}
		return true;
	}
};

void read_gameplay_json(Gameplay& gameplay, Json& json) {
	Gameplay temp = from_json<Gameplay>(json);
	GameplayMerger merger{&gameplay, &temp};
	gameplay.enumerate_fields(merger);
}

template <typename Object>
void write_gameplay_section(Json& dest, std::optional<Object>& src, const char* name) {
	if(!src.has_value()) {
		dest[name] = {};
		return;
	}
	dest[name] = to_json(*src);
}

template <typename Object>
void write_gameplay_section(Json& dest, std::optional<std::vector<Object>>& src, const char* name) {
	if(!src.has_value()) {
		dest[name] = {};
		return;
	}
	for(Object& object : *src) {
		dest[name].emplace_back(to_json(object));
	}
}

Json write_gameplay_json(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "gameplay"},
		{"format_version", 0},
		{"application", "Wrench WAD Utility"},
		{"application_version", get_application_version_string()}
	};
	
	Json data = to_json(gameplay);
	for(auto& item : data.items()) {
		json[item.key()] = item.value();
	}
	
	return json;
}

void read_help_messages(Gameplay& gameplay, Json& json) {
	
}

Json write_help_messages(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "help_messages"},
		{"format_version", 0},
		{"application", "Wrench WAD Utility"},
		{"application_version", get_application_version_string()}
	};
	
	struct {
		const char* name;
		Opt<std::vector<HelpMessage>>* messages;
	} languages[8] = {
		{"us_english", &gameplay.us_english_help_messages},
		{"uk_english", &gameplay.uk_english_help_messages},
		{"french", &gameplay.french_help_messages},
		{"german", &gameplay.german_help_messages},
		{"spanish", &gameplay.spanish_help_messages},
		{"italian", &gameplay.italian_help_messages},
		{"japanese", &gameplay.japanese_help_messages},
		{"korean", &gameplay.korean_help_messages}
	};
	for(auto& language : languages) {
		if(language.messages->has_value()) {
			for(HelpMessage& message : **language.messages) {
				json[language.name].emplace_back(to_json(message));
			}
		}
	}
	return json;
}

void fixup_pvar_indices(Gameplay& gameplay) {
	s32 pvar_index = 0;
	if(gameplay.cameras.has_value()) {
		for(ImportCamera& camera : *gameplay.cameras) {
			if(camera.pvars.size() > 0) {
				camera.pvar_index = pvar_index++;
			} else {
				camera.pvar_index = -1;
			}
		}
	}
	if(gameplay.sound_instances.has_value()) {
		for(SoundInstance& inst : *gameplay.sound_instances) {
			if(inst.pvars.size() > 0) {
				inst.pvar_index = pvar_index++;
			} else {
				inst.pvar_index = -1;
			}
		}
	}
	if(gameplay.moby_instances.has_value()) {
		for(MobyInstance& inst : *gameplay.moby_instances) {
			if(inst.pvars.size() > 0) {
				inst.pvar_index = pvar_index++;
			} else {
				inst.pvar_index = -1;
			}
		}
	}
}
