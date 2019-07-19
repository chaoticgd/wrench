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

namespace fs = boost::filesystem;

iso_stream::iso_stream(std::string game_id, std::string iso_path, worker_logger& log)
	: iso_stream(game_id, iso_path, log, nullptr) {}

iso_stream::iso_stream(std::string game_id, std::string iso_path, worker_logger& log, ZipArchive::Ptr root)
	: _iso(iso_path),
	  _patches(read_patches(root)),
	  _cache_iso_path(std::string("cache/editor_") + game_id + "_patched.iso"),
	  _cache_meta_path(std::string("cache/editor_") + game_id + "_metadata.json"),
	  _cache(init_cache(iso_path, log), std::ios::in | std::ios::out) {}

uint32_t iso_stream::size() {
	return _cache.size();
}

void iso_stream::seek(uint32_t offset) {
	_cache.seek(offset);
}

uint32_t iso_stream::tell() {
	return _cache.tell();
}

void iso_stream::read_n(char* dest, uint32_t size) {
	return _cache.read_n(dest, size);
}

void iso_stream::write_n(const char* data, uint32_t size) {
	_patches.emplace_back(tell(), std::vector<char>(size));
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

void iso_stream::commit() {
	// Stub
}

void iso_stream::save_patches(ZipArchive::Ptr& root, std::string wratch_path) {
	std::vector<nlohmann::json> patch_list;
	std::vector<std::unique_ptr<std::stringstream>> patch_streams;
	for(std::size_t i = 0; i < _patches.size(); i++) {
		auto patch = _patches[i];
		std::string name = std::string("patches/") + std::to_string(i) + ".bin";
		auto patch_bin = root->CreateEntry(name);
		patch_streams.emplace_back(std::make_unique<std::stringstream>());
		patch_streams.back()->write(patch.buffer.data(), patch.buffer.size());
		patch_bin->SetCompressionStream(*patch_streams.back().get());
		patch_list.emplace_back(nlohmann::json {
			{ "offset", patch.offset },
			{ "size", patch.buffer.size() },
			{ "data", name }
		});
	}

	nlohmann::json patch_list_file;
	patch_list_file["patches"] = patch_list;

	std::stringstream patch_list_stream;
	patch_list_stream << patch_list_file.dump(4);

	root->CreateEntry("patch_list.json")->SetCompressionStream(patch_list_stream);
	ZipFile::SaveAndClose(root, wratch_path);
}

std::vector<patch> iso_stream::read_patches(ZipArchive::Ptr root) {
	if(root.get() == nullptr) {
		return {}; // New project. Nothing to do.
	}

	std::istream* patch_list_file = root->GetEntry("patch_list.json")->GetDecompressionStream();
	auto patch_list = nlohmann::json::parse(*patch_list_file);

	std::vector<patch> result;
	for(auto& p : patch_list.find("patches").value()) {
		result.emplace_back(
			p.find("offset").value().operator uint32_t(),
			std::vector<char>(p.find("size").value().operator std::size_t())
		);
		std::string patch_src_path = p.find("data").value();
		std::istream* patch_file = root->GetEntry(patch_src_path)->GetDecompressionStream();
		patch_file->read(result.back().buffer.data(), result.back().buffer.size());
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
		write_all_patches(&cache_iso);
	} else {
		log << "[ISO] Rebuilding cache... ";

		// The cache is invalid.
		fs::remove(_cache_iso_path);
		fs::remove(_cache_meta_path);
		fs::copy(iso_path, _cache_iso_path);

		file_stream cache_iso(_cache_iso_path, std::ios::in | std::ios::out);
		write_all_patches(&cache_iso);
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
		uint32_t offset = static_cast<uint32_t>(patch["offset"].get<uint64_t>());
		uint32_t size = static_cast<uint32_t>(patch["size"].get<uint64_t>());

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

void iso_stream::write_all_patches(file_stream* cache_iso) {
	for(patch& p : _patches) {
		cache_iso->seek(p.offset);
		cache_iso->write_n(p.buffer.data(), p.buffer.size());
	}
}

std::string iso_stream::hash_patches() {
	MD5_CTX ctx;
	MD5Init(&ctx);

	for(patch& p : _patches) {
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(&p.offset), 4);
		uint32_t size = p.buffer.size();
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(&size), 4);
		MD5Update(&ctx, reinterpret_cast<uint8_t*>(p.buffer.data()), p.buffer.size());
	}

	std::string digest;
	digest.resize(MD5_DIGEST_LENGTH);
	MD5Final(reinterpret_cast<uint8_t*>(digest.data()), &ctx);

	std::stringstream result;
	for(std::size_t i = 0; i < MD5_DIGEST_LENGTH; i++) {
		result << std::hex << (digest[i] & 0xff);
	}

	return result.str();
}
