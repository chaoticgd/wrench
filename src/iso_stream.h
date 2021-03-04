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

#ifndef ISO_STREAM_H
#define ISO_STREAM_H

#include <nlohmann/json.hpp>
#include <ZipLib/ZipArchive.h>
#include <ZipLib/ZipFile.h>

#include "stream.h"
#include "worker_logger.h"

# /*
#	Generates patches from a series of write_n calls made by the
#	rest of the program and loading patches from .wrench files.
# */

class simple_wad_stream : public array_stream {
public:
	simple_wad_stream(stream* backing, size_t offset);
};

// All code below this point is obsolete and should be removed at some point.

struct patch {
	patch() {}
	
	std::size_t offset;
	std::vector<char> buffer;
	bool save_to_project;
};

struct wad_patch {
	wad_patch() {}
	
	std::size_t offset;
	std::vector<char> buffer;
};

class iso_stream;

class wad_stream : public stream {
	friend iso_stream;
public:
	wad_stream(iso_stream* backing, std::size_t offset, std::vector<wad_patch> patches);

	std::size_t size() const override;
	void seek(std::size_t offset) override;
	std::size_t tell() const override;
 	void read_n(char* dest, std::size_t size) override;
	void write_n(const char* data, std::size_t size) override;
	std::string resource_path() const override;
	
	void commit();

	// We don't want to recompress some WAD segments right now for speed.
	// This is the switch for that.
	bool discard = false;

private:
	iso_stream* _backing;
	std::size_t _offset;
	array_stream _uncompressed_buffer;
	std::vector<wad_patch> _wad_patches;
	bool _dirty; // Does the segment need to be recompressed?
};

class iso_stream : public stream {
	friend wad_stream;
public:
	iso_stream(std::string game_id, std::string iso_path, worker_logger& log); // New Project
	iso_stream(std::string game_id, std::string iso_path, worker_logger& log, ZipArchive::Ptr root); // Open Project

	std::size_t size() const override;
	void seek(std::size_t offset) override;
	std::size_t tell() const override;
 	void read_n(char* dest, std::size_t size) override;
	void write_n(const char* data, std::size_t size) override;
	void write_n(const char* data, std::size_t size, bool save_to_project);
	std::string resource_path() const override;


	std::string cached_iso_path() const;

	// Save patches to .wrench file.
	void save_patches_to_and_close(ZipArchive::Ptr& root, std::string project_path);

	// Decompress a WAD segment. Register the stream so that the segment can be
	// automatically recompressed when changes need to be commited to the cache.
	wad_stream* get_decompressed(std::size_t offset, bool discard = false);
	
	// Recompress all open WAD segments.
	void commit();

	file_stream _iso;
	
private:

	std::vector<patch> read_patches(ZipArchive::Ptr root);
	std::map<std::size_t, std::unique_ptr<wad_stream>> read_wad_streams(ZipArchive::Ptr root);

	std::string init_cache(std::string iso_path, worker_logger& log);
	std::optional<nlohmann::json> get_cache_metadata();

	// Remove all patches from the cache ISO (does not affect metadata).
	void clear_cache_iso(file_stream* cache_iso); // May be called before _cache is initialised.

	// Write a hash of the current patches and the ranges that were patched
	// out to a file.
	void update_cache_metadata();

	// Write patches in _patches to the cache ISO.
	void write_normal_patches(file_stream* cache_iso);

	// Generate a hash based on _patches.
	std::string hash_patches();

	std::vector<patch> _patches;
	std::map<std::size_t, std::unique_ptr<wad_stream>> _wad_streams;

	std::string _cache_iso_path;
	std::string _cache_meta_path;
	file_stream _cache; // Must be initialised last.
};

std::string md5_from_stream(stream& st);

#endif
