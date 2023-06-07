/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2022 chaoticgd

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

#include "zipped_asset_bank.h"

ZippedAssetBank::ZippedAssetBank(AssetForest& forest, const char* zip_path, fs::path prefix)
	: AssetBank(forest, false)
	, _prefix(std::move(prefix)) {
	int error;
	_zip = zip_open(zip_path, ZIP_RDONLY, &error);
	verify(_zip, "Failed to open zip file.");
	// If no prefix was explicitly provided and there's no gameinfo.txt in the
	// root directory, try and find the first gameinfo.txt file and use that.
	if(_prefix.empty() && !file_exists(_prefix/"gameinfo.txt")) {
		s64 count = zip_get_num_entries(_zip, 0);
		for(s32 i = 0; i < count; i++) {
			if(const char* name = zip_get_name(_zip, i, 0)) {
				fs::path path(name);
				if(path.filename() == "gameinfo.txt") {
					_prefix = path.parent_path();
					break;
				}
			}
		}
	}
}

ZippedAssetBank::~ZippedAssetBank() {
	zip_close(_zip);
}

std::unique_ptr<InputStream> ZippedAssetBank::open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const {
	fs::path absolute_path = _prefix/path;
	zip_stat_t stat;
	verify(zip_stat(_zip, absolute_path.string().c_str(), 0, &stat) == 0, "Failed to open zipped file '%s'.", absolute_path.string().c_str());
	verify(stat.valid & ZIP_STAT_SIZE, "Failed to find size of zipped file '%s'.", absolute_path.string().c_str());
	std::unique_ptr<ZipInputStream> stream = std::make_unique<ZipInputStream>();
	if(stream->open(_zip, absolute_path.string().c_str(), (s64) stat.size)) {
		return stream;
	} else {
		return nullptr;
	}
}

std::unique_ptr<OutputStream> ZippedAssetBank::open_binary_file_for_writing(const fs::path& path) {
	verify_not_reached("Tried to write to a zipped asset bank!");
}

std::string ZippedAssetBank::read_text_file(const fs::path& path) const {
	auto stream = open_binary_file_for_reading(path, nullptr);
	s64 size = stream->size();
	std::string data;
	data.resize(size);
	stream->read_n((u8*) data.data(), size);
	strip_carriage_returns_from_string(data);
	return data;
}

void ZippedAssetBank::write_text_file(const fs::path& path, const char* contents) {
	verify_not_reached("Tried to write to a zipped asset bank!");
}

bool ZippedAssetBank::file_exists(const fs::path& path) const {
	zip_stat_t dummy;
	return zip_stat(_zip, (_prefix/path).string().c_str(), 0, &dummy) == 0;
}

std::vector<fs::path> ZippedAssetBank::enumerate_asset_files() const {
	std::vector<fs::path> asset_files;
	s64 count = zip_get_num_entries(_zip, 0);
	for(s64 i = 0; i < count; i++) {
		if(const char* name = zip_get_name(_zip, i, 0)) {
			fs::path path = fs::path(name).lexically_relative(_prefix);
			if(!path.string().starts_with("..") && path.extension() == ".asset") {
				asset_files.emplace_back(path);
			}
		}
	}
	return asset_files;
}

std::vector<fs::path> ZippedAssetBank::enumerate_source_files() const {
	std::string common_source_path = get_common_source_path();
	std::string game_source_path = get_game_source_path();
	
	std::vector<fs::path> asset_files;
	s64 count = zip_get_num_entries(_zip, 0);
	for(s64 i = 0; i < count; i++) {
		if(const char* name = zip_get_name(_zip, i, 0)) {
			fs::path path = fs::path(name).lexically_relative(_prefix);
			if(path.string().starts_with(common_source_path) || path.string().starts_with(game_source_path)) {
				asset_files.emplace_back(path);
			}
		}
	}
	return asset_files;
}

s32 ZippedAssetBank::check_lock() const {
	return 0;
}

void ZippedAssetBank::lock() {}

// *****************************************************************************

ZipInputStream::~ZipInputStream() {
	if(_file) {
		zip_fclose(_file);
	}
}
	
bool ZipInputStream::open(zip_t* zip, const char* path, s64 size) {
	_file = zip_fopen(zip, path, 0);
	_size = size;
	return _file != nullptr;
}

bool ZipInputStream::seek(s64 offset) {
	return zip_fseek(_file, offset, SEEK_SET) == 0;
}

s64 ZipInputStream::tell() const {
	return zip_ftell(_file);
}

s64 ZipInputStream::size() const {
	return _size;
}

bool ZipInputStream::read_n(u8* dest, s64 size) {
	return zip_fread(_file, dest, size) == size;
}
