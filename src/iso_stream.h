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

#include <nlohmann/json.hpp>
#include <ZipLib/ZipArchive.h>
#include <ZipLib/ZipFile.h>

#include "stream.h"
#include "worker_logger.h"

# /*
#	Generates patches from a series of write_n calls made by the
#	rest of the program and loading patches from .wrench files.
# */

struct patch {
	patch() {}

	patch(uint32_t offset_, std::vector<char> buffer_)
		: offset(offset_), buffer(buffer_) {} 

	uint32_t offset;
	std::vector<char> buffer;
};

class iso_stream : public stream {
public:
	iso_stream(std::string game_id, std::string iso_path, worker_logger& log); // New Project
	iso_stream(std::string game_id, std::string iso_path, worker_logger& log, ZipArchive::Ptr root); // Open Project

	uint32_t size() const override;
	void seek(uint32_t offset) override;
	uint32_t tell() const override;
 	void read_n(char* dest, uint32_t size) override;
	void write_n(const char* data, uint32_t size) override;

	std::string resource_path() const override;

	std::string cached_iso_path() const;

	// Save patches to .wratch file.
	void save_patches(ZipArchive::Ptr& root, std::string project_path);

private:

	std::vector<patch> read_patches(ZipArchive::Ptr root);

	std::string init_cache(std::string iso_path, worker_logger& log);
	std::optional<nlohmann::json> get_cache_metadata();

	// Remove all patches from the cache ISO (does not affect metadata).
	void clear_cache_iso(file_stream* cache_iso); // May be called before _cache is initialised.

	// Apply a new patch to the cache ISO.
	void update_cache_metadata();

	// Write all patches in _patches to the cache ISO.
	void write_all_patches(file_stream* cache_iso);

	// Generate a hash based on _patches.
	std::string hash_patches();

	file_stream _iso;
	std::vector<patch> _patches;

	std::string _cache_iso_path;
	std::string _cache_meta_path;
	file_stream _cache; // Must be initialised last.
};
