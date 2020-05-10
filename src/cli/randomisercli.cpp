/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2020 chaoticgd

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

#include <ctime>
#include <cstdio>
#include <random>
#include <string>
#include <cstdlib>
#include <algorithm>

#include "../md5.h"
#include "../game_db.h"
#include "../project.h"
#include "../command_line.h"
#include "../formats/level_impl.h"

// Read an array, shuffle its elements, then write it back.
template <typename T>
void read_shuffle_write(stream& backing, std::size_t offset, std::size_t num_elements);

int main(int argc, char** argv) {
	cxxopts::Options options("randomiser", "Randomise textures");
	options.add_options()
		("i,iso", "The game ISO to use.",
			cxxopts::value<std::string>())
		("g,gameid", "The game ID (e.g. 'SCES_516.07')",
			cxxopts::value<std::string>())
		("p,project", "The path of the new project to create.",
			cxxopts::value<std::string>())
		("s,seed", "Password to seed the random number generator.",
			cxxopts::value<std::string>());

	options.parse_positional({
		"iso", "gameid", "project", "seed"
	});

	auto args = parse_command_line_args(argc, argv, options);
	std::string iso_path = cli_get(args, "iso");
	std::string game_id = cli_get(args, "gameid");
	std::string project_path = cli_get(args, "project");
	std::string password = cli_get_or(args, "seed", "");
	
	if(password == "") {
		srand(time(0));
		password = int_to_hex(rand());
	}
	
	printf("Seed: %s\n", password.c_str());
	
	MD5_CTX ctx;
	MD5Init(&ctx);
	MD5Update(&ctx, (const uint8_t*) password.data(), password.size());
	uint8_t hash[MD5_DIGEST_LENGTH];
	MD5Final(hash, &ctx);
	srand(*(unsigned int*) hash);
	
	auto game = gamedb_parse_file().at(game_id);
	
	// TODO: Change the wrench_project constructor so this mess isn't required.
	std::map<std::string, std::string> game_paths
		{ { game_id, iso_path } };
	
	worker_logger log;
	wrench_project project(game_paths, log, game_id);
	
	for(gamedb_file file_meta : game.files) {
		proxy_stream file(&project.iso, file_meta.offset, file_meta.size);
		
		if(file_meta.type == +gamedb_file_type::LEVEL) {
			level_file_header file_header = level::read_file_header(&file);
			auto primary_header = file.read<level::fmt::primary_header>(file_header.primary_header_offset);
			
			std::size_t asset_header_offset = file_header.primary_header_offset + primary_header.snd_header.value;
			auto asset_header = file.read<level::fmt::secondary_header>(asset_header_offset);
			
			packed_struct(texture_entry,
				uint32_t field_0;
				uint32_t field_4;
				uint32_t field_8;
				uint32_t field_c;
			);
			
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.terrain_texture_offset,
				asset_header.terrain_texture_count);
				
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.some1_texture_offset,
				asset_header.some1_texture_count);
			
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.shrub_texture_offset,
				asset_header.shrub_texture_count);
			
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.tie_texture_offset,
				asset_header.tie_texture_count);
			
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.some2_texture_offset,
				asset_header.some2_texture_count);
			
			read_shuffle_write<texture_entry>(
				file,
				asset_header_offset + asset_header.sprite_texture_offset,
				asset_header.sprite_texture_count);
		}
	}
	
	project.save_to(project_path);
}

template <typename T>
void read_shuffle_write(stream& backing, std::size_t offset, std::size_t num_elements) {
	std::vector<T> elements(num_elements);
	backing.seek(offset);
	backing.read_v(elements);
	std::random_device rng;
	std::mt19937 urng(rng());
	std::shuffle(elements.begin(), elements.end(), urng);
	backing.seek(offset);
	backing.write_v(elements);
}
