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

#ifndef ASSETMGR_ZIPPED_ASSET_BANK_H
#define ASSETMGR_ZIPPED_ASSET_BANK_H

#include <zip.h>

#include "asset.h"

class ZippedAssetBank : public AssetBank
{
public:
	ZippedAssetBank(AssetForest& forest, const char* zip_path, fs::path prefix = "");
	~ZippedAssetBank();
	
private:
	std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const override;
	std::unique_ptr<OutputStream> open_binary_file_for_writing(const fs::path& path) override;
	std::string read_text_file(const fs::path& path) const override;
	void write_text_file(const fs::path& path, const char* contents) override;
	bool file_exists(const fs::path& path) const override;
	std::vector<fs::path> enumerate_asset_files() const override;
	void enumerate_source_files(std::map<fs::path, const AssetBank*>& dest, Game game) const override;
	s32 check_lock() const override;
	void lock() override;
	
	zip_t* m_zip;
	fs::path m_prefix;
};

class ZipInputStream : public InputStream
{
public:
	ZipInputStream() {}
	~ZipInputStream();
	
	bool open(zip_t* zip, const char* path, s64 size);
	
	bool seek(s64 offset) override;
	s64 tell() const override;
	s64 size() const override;
	
	bool read_n(u8* dest, s64 size) override;
	
private:
	zip_file_t* m_file = nullptr;
	s64 m_size;
};

#endif
