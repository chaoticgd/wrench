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

Json write_gameplay_json(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "gameplay"},
		{"format_version", 0},
		{"application", "Wrench WAD Extractor"},
		{"application_version", get_application_version_string()}
	};
	
	Json data = to_json(gameplay);
	for(auto& object : data.items()) {
		json[object.key()] = object.value();
	}
	
	return json;
}

Json write_help_messages(Gameplay& gameplay) {
	Json json;
	
	json["metadata"] = Json {
		{"format", "help_messages"},
		{"format_version", 0},
		{"application", "Wrench WAD Extractor"},
		{"application_version", get_application_version_string()}
	};
	
	struct {
		const char* name;
		Opt<std::vector<GpHelpMessage>>* messages;
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
			for(GpHelpMessage& message : **language.messages) {
				json[language.name].emplace_back(to_json(message));
			}
		}
	}
	return json;
}