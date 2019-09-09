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

#include "iso_stream.h"

#include <boost/filesystem.hpp>

#include "md5.h"
#include "util.h"
#include "formats/wad.h"

namespace fs = boost::filesystem;

wad_stream::wad_stream(iso_stream* backing, std::size_t offset, std::vector<wad_patch> patches)
	: _backing(backing),
	  _offset(offset),
	  _wad_patches(patches),
	  _dirty(true) {
	// Read in the stock WAD.
	proxy_stream segment(&_backing->_iso, _offset, 0);
	decompress_wad_cached(_uncompressed_buffer, segment);
	
	// Apply patches from project file.
	for(auto& p : patches) {
		_uncompressed_buffer.seek(p.offset);
		_uncompressed_buffer.write_n(p.buffer.data(), p.buffer.size());
	}
}

std::size_t wad_stream::size() const {
	return _uncompressed_buffer.size();
}

void wad_stream::seek(std::size_t offset) {
	_uncompressed_buffer.seek(offset);
}

std::size_t wad_stream::tell() const {
	return _uncompressed_buffer.tell();
}

void wad_stream::read_n(char* dest, std::size_t size) {
	_uncompressed_buffer.read_n(dest, size);
}

void wad_stream::write_n(const char* data, std::size_t size) {
	_wad_patches.emplace_back();
	_wad_patches.back().offset = tell();
	_wad_patches.back().buffer = std::vector<char>(data, data + size);
	_uncompressed_buffer.write_n(data, size);
	_dirty = true;
}

std::string wad_stream::resource_path() const {
	proxy_stream segment(_backing, _offset, 0);
	return std::string("wad(") + segment.resource_path() + ")";
}

void wad_stream::commit() {
	if(!_dirty) {
		return; // The segment hasn't been modified since the last time it was committed.
	}
	_dirty = false;
	
	array_stream compressed_buffer;
	_uncompressed_buffer.seek(0);
	compress_wad(compressed_buffer, _uncompressed_buffer);
	
	compressed_buffer.seek(0);
	_backing->seek(_offset);
	_backing->write_n(compressed_buffer.data(), compressed_buffer.size(), false);
}

iso_stream::iso_stream(std::string game_id, std::string iso_path, worker_logger& log)
	: iso_stream(game_id, iso_path, log, nullptr) {}

iso_stream::iso_stream(std::string game_id, std::string iso_path, worker_logger& log, ZipArchive::Ptr root)
	: _iso(iso_path),
	  _patches(read_patches(root)),
	  _wad_streams(read_wad_streams(root)),
	  _cache_iso_path(std::string("cache/editor_") + game_id + "_patched.iso"),
	  _cache_meta_path(std::string("cache/editor_") + game_id + "_metadata.json"),
	  _cache(init_cache(iso_path, log), std::ios::in | std::ios::out) {}

std::size_t iso_stream::size() const {
	return _cache.size();
}

void iso_stream::seek(std::size_t offset) {
	_cache.seek(offset);
}

std::size_t iso_stream::tell() const {
	return _cache.tell();
}

void iso_stream::read_n(char* dest, std::size_t size) {
	return _cache.read_n(dest, size);
}

void iso_stream::write_n(const char* data, std::size_t size) {
	write_n(data, size, true);
}

void iso_stream::write_n(const char* data, std::size_t size, bool save_to_project) {
	_patches.emplace_back();
	_patches.back().offset = tell();
	_patches.back().buffer = std::vector<char>(size);
	_patches.back().save_to_project = save_to_project;
	std::memcpy(_patches.back().buffer.data(), data, size);
	_cache.write_n(data, size);
	update_cache_metadata();
}

std::string iso_stream::resource_path() const {
	return "iso";
}

std::string iso_stream::cached_iso_path() const {
	return _cache_iso_path;
}

void iso_stream::save_patches_to_and_close(ZipArchive::Ptr& root, std::string project_path) {
	std::vector<std::unique_ptr<std::stringstream>> patch_streams;
	
	std::vector<nlohmann::json> patch_list;
	for(std::size_t i = 0; i < _patches.size(); i++) {
		auto patch = _patches[i];
		if(!patch.save_to_project) {
			continue;
		}
		std::string name = std::string("patches/") + std::to_string(i) + ".bin";
		auto patch_bin = root->CreateEntry(name);
		patch_streams.emplace_back(std::make_unique<std::stringstream>());
		patch_streams.back()->write(patch.buffer.data(), patch.buffer.size());
		patch_bin->SetCompressionStream(*patch_streams.back().get());
		patch_list.emplace_back(nlohmann::json {
			{ "offset", patch.offset },
			{ "data", name }
		});
	}

	std::map<std::string, nlohmann::json> wad_patch_list;
	for(auto& [wad_offset, wad] : _wad_streams) {
		std::vector<nlohmann::json> wad_json;
		for(std::size_t i = 0; i < wad->_wad_patches.size(); i++) {
			auto& current = wad->_wad_patches[i];
			std::string name =
				std::string("wad_patches/") +
				int_to_hex(wad_offset) + "_" +
				std::to_string(i) + ".bin";
			auto patch_bin = root->CreateEntry(name);
			patch_streams.emplace_back(std::make_unique<std::stringstream>());
			patch_streams.back()->write(current.buffer.data(), current.buffer.size());
			patch_bin->SetCompressionStream(*patch_streams.back().get());
			wad_json.emplace_back(nlohmann::json {
				{ "offset", current.offset },
				{ "data", name }
			});
		}
		
		wad_patch_list[int_to_hex(wad_offset)] = wad_json;
	}

	nlohmann::json patch_list_file;
	patch_list_file["patches"] = patch_list;
	patch_list_file["wad_patches"] = wad_patch_list;

	std::stringstream patch_list_stream;
	patch_list_stream << patch_list_file.dump(4);

	root->CreateEntry("patch_list.json")->SetCompressionStream(patch_list_stream);
	
	// Required since patch_streams will be destructed upon returning.
	ZipFile::SaveAndClose(root, project_path);
}

wad_stream* iso_stream::get_decompressed(std::size_t offset) {
	if(_wad_streams.find(offset) == _wad_streams.end()) {
		// The segment hasn't been patched yet.
		_wad_streams.emplace(offset, std::make_unique<wad_stream>
			(this, offset, std::vector<wad_patch>()));
	}
	return _wad_streams.at(offset).get();
}

void iso_stream::commit() {
	for(auto& wad : _wad_streams) {
		wad.second->commit();
	}
}

std::vector<patch> iso_stream::read_patches(ZipArchive::Ptr root) {
	if(root.get() == nullptr) {
		return {}; // New project. Nothing to do.
	}

	auto patch_list_entry = root->GetEntry("patch_list.json");
	std::istream* patch_list_file = patch_list_entry->GetDecompressionStream();
	auto patch_list = nlohmann::json::parse(*patch_list_file);

	std::vector<patch> result;
	for(auto& p : patch_list.find("patches").value()) {
		result.emplace_back();
		result.back().offset = p.find("offset").value().operator std::size_t();
		result.back().buffer = std::vector<char>(p.find("size").value().operator std::size_t());
		std::string patch_src_path = p.find("data").value();
		std::istream* patch_file = root->GetEntry(patch_src_path)->GetDecompressionStream();
		patch_file->read(result.back().buffer.data(), result.back().buffer.size());
	}
	
	patch_list_entry->CloseDecompressionStream();

	return result;
}

std::map<std::size_t, std::unique_ptr<wad_stream>> iso_stream::read_wad_streams(ZipArchive::Ptr root) {
	if(root.get() == nullptr) {
		return {}; // New project. Nothing to do.
	}
	
	std::map<std::size_t, std::unique_ptr<wad_stream>> result;
	
	auto entry = root->GetEntry("patch_list.json");
	std::istream* patch_list_file = entry->GetDecompressionStream();
	
	auto patch_list = nlohmann::json::parse(*patch_list_file);
	if(patch_list.find("wad_patches") == patch_list.end()) {
		return {};
	}
	
	for(auto& [wad_offset_str, wad] : patch_list.find("wad_patches").value().get<nlohmann::json::object_t>()) {
		std::vector<wad_patch> wad_patches;
		
		for(auto& patch_json : wad.items()) {
			std::string patch_src_path = patch_json.value().find("data").value();
			auto bin_entry = root->GetEntry(patch_src_path);
			std::istream* patch_file = bin_entry->GetDecompressionStream();
			
			std::vector<char> buffer(bin_entry->GetSize());
			patch_file->read(buffer.data(), buffer.size());
			
			wad_patches.emplace_back();
			wad_patches.back().offset = patch_json.value()["offset"];
			wad_patches.back().buffer = buffer;
		}
		
		std::size_t wad_offset = hex_to_int(wad_offset_str);
		result.emplace(wad_offset, std::make_unique<wad_stream>
			(this, wad_offset, wad_patches));
	}
	
	return result;
}

std::string iso_stream::init_cache(std::string iso_path, worker_logger& log) {
	fs::create_directory("cache");

	if(auto cache_meta = get_cache_metadata()) {
		std::string patch_hash = hash_patches();
		if((*cache_meta)["hash"].get<std::string>() == patch_hash) {
			// The cache is valid. Do nothing.
			return _cache_iso_path;
		}

		log << "[ISO] Updating cache... ";

		// The cache needs updating.
		file_stream cache_iso(_cache_iso_path, std::ios::in | std::ios::out);
		clear_cache_iso(&cache_iso);
		write_normal_patches(&cache_iso);
	} else {
		log << "[ISO] Rebuilding cache... ";

		// The cache is invalid.
		fs::remove(_cache_iso_path);
		fs::remove(_cache_meta_path);
		fs::copy(iso_path, _cache_iso_path);

		file_stream cache_iso(_cache_iso_path, std::ios::in | std::ios::out);
		write_normal_patches(&cache_iso);
	}

	update_cache_metadata();
	log << "DONE!\n";

	return _cache_iso_path;
}

std::optional<nlohmann::json> iso_stream::get_cache_metadata() {
	if(!fs::exists(_cache_iso_path) || !fs::exists(_cache_meta_path)) {
		return {};
	}

	std::ifstream json_file(_cache_meta_path);
	try {
		auto json = nlohmann::json::parse(json_file);
		if(!json["hash"].is_string()) return {};
		if(!json["patches"].is_array()) return {};
		return json;
	} catch(...) {
		return {};
	}
}

void iso_stream::clear_cache_iso(file_stream* cache_iso) {
	std::ifstream cache_meta_file(_cache_meta_path);
	nlohmann::json cache_meta = nlohmann::json::parse(cache_meta_file);
	for(auto& patch : cache_meta["patches"]) {
		std::size_t offset = static_cast<std::size_t>(patch["offset"].get<uint64_t>());
		std::size_t size = static_cast<std::size_t>(patch["size"].get<uint64_t>());

		_iso.seek(offset);
		cache_iso->seek(offset);
		stream::copy_n(*cache_iso, _iso, size);
	}
}

void iso_stream::update_cache_metadata() {
	std::vector<nlohmann::json> patches_json;

	for(std::size_t i = 0; i < _patches.size(); i++) {
		patches_json.push_back({
			{ "offset", _patches[i].offset },
			{ "size", _patches[i].buffer.size() }
		});
	}

	nlohmann::json cache_meta = {};
	cache_meta["hash"] = hash_patches();
	cache_meta["patches"] = patches_json;
	
	auto metadata_str = cache_meta.dump(4);
	std::ofstream metadata(_cache_meta_path, std::ios::trunc);
	metadata.write(metadata_str.data(), metadata_str.size());
}

void iso_stream::write_normal_patches(file_stream* cache_iso) {
	for(patch& p : _patches) {
		cache_iso->seek(p.offset);
		cache_iso->write_n(p.buffer.data(), p.buffer.size());
	}
}

std::string md5_to_printable_string(std::array<uint8_t, MD5_DIGEST_LENGTH> in) {
	std::stringstream result;
	for(std::size_t i = 0; i < MD5_DIGEST_LENGTH; i++) {
		result << std::hex << (in[i] & 0xff);
	}
	return result.str();
}

std::string iso_stream::hash_patches() {
	MD5_CTX ctx;
	MD5Init(&ctx);

	for(patch& p : _patches) {
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(&p.offset), 4);
		std::size_t size = p.buffer.size();
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(&size), 4);
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(p.buffer.data()), p.buffer.size());
	}

	std::array<uint8_t, MD5_DIGEST_LENGTH> digest;
	MD5Final(digest.data(), &ctx);
	return md5_to_printable_string(digest);
}

void decompress_wad_cached(stream& dest, stream& src) {
	MD5_CTX ctx;
	MD5Init(&ctx);
	
	std::vector<char> buffer(src.read<uint32_t>(3)); // src.size() is unreliable.
	src.seek(0);
	src.read_v(buffer);
	MD5Update(&ctx, reinterpret_cast<uint8_t*>(buffer.data()), buffer.size());
	
	std::array<uint8_t, MD5_DIGEST_LENGTH> digest;
	MD5Final(digest.data(), &ctx);
	std::string md5 = md5_to_printable_string(digest);
	
	auto cache_path = std::string("cache/wad_") + md5 + ".bin";
	if(fs::exists(cache_path)) {
		file_stream cache_file(cache_path);
		dest.seek(0);
		stream::copy_n(dest, cache_file, cache_file.size());
	} else {
		decompress_wad(dest, src);
		file_stream cache_file(cache_path, std::ios::out);
		dest.seek(0);
		stream::copy_n(cache_file, dest, dest.size());
	}
}
