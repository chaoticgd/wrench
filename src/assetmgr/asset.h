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

#ifndef ASSETMGR_ASSET_H
#define ASSETMGR_ASSET_H

// This file and its associated .cpp file contains the core of Wrench's asset
// system. The top-level object here is the asset forest, which contains asset
// packs, which contain .asset files, which contain a tree of assets. Each mod
// as well as each base game here would be represented by an asset pack.

#include <memory>
#include <functional>

#include <core/wtf.h>
#include <core/stream.h>
#include <core/wtf_writer.h>
#include <assetmgr/asset_util.h>

class Asset {
protected:
	Asset(AssetForest& forest, AssetPack& pack, AssetFile& file, Asset* parent, AssetType type, std::string tag);
public:
	Asset(const Asset&) = delete;
	Asset(Asset&&) = delete;
	virtual ~Asset();
	Asset& operator=(const Asset&) = delete;
	Asset& operator=(Asset&&) = delete;
	
	AssetForest& forest();
	const AssetForest& forest() const;
	AssetPack& pack();
	const AssetPack& pack() const;
	AssetFile& file();
	const AssetFile& file() const;
	Asset* parent();
	const Asset* parent() const;
	AssetType type() const;
	const std::string& tag() const;
	Asset* lower_precedence();
	Asset* higher_precedence();
	Asset* lowest_precedence();
	Asset* highest_precedence();
	AssetReference absolute_reference() const;
	AssetReference reference_relative_to(Asset& asset) const;
	
	template <typename Callback>
	void for_each_physical_child(Callback callback) {
		for(std::unique_ptr<Asset>& child : _children) {
			callback(*child.get());
		}
	}
	
	template <typename Callback>
	void for_each_physical_child(Callback callback) const {
		for(const std::unique_ptr<Asset>& child : _children) {
			callback(*child.get());
		}
	}
	
	template <typename Callback>
	void for_each_logical_child(Callback callback) {
		for(Asset* asset = lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->_children) {
				if(child->lower_precedence() == nullptr) {
					callback(*child.get());
				}
			}
		}
	}
	
	template <typename ChildType, typename Callback>
	void for_each_logical_child_of_type(Callback callback) {
		for(Asset* asset = lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->_children) {
				if(child->lower_precedence() == nullptr && child->type() == ChildType::ASSET_TYPE) {
					callback(static_cast<ChildType&>(*child.get()));
				}
			}
		}
	}
	
	template <typename ChildType>
	ChildType& child(const char* tag) {
		Asset& asset = physical_child(ChildType::ASSET_TYPE, tag);
		return asset.as<ChildType>();
	}
	
	template <typename ChildType>
	ChildType& child(s32 tag) {
		std::string str = std::to_string(tag);
		Asset& asset = physical_child(ChildType::ASSET_TYPE, str.c_str());
		return asset.as<ChildType>();
	}
	
	template <typename AssetType>
	AssetType& as() {
		return dynamic_cast<AssetType&>(*this);
	}
	
	bool has_child(const char* tag);
	bool has_child(s32 tag);
	Asset& get_child(const char* tag);
	Asset& get_child(s32 tag);
	
	Asset& physical_child(AssetType type, const char* tag);
	Asset* get_physical_child(const char* tag);
	bool remove_physical_child(Asset& asset);
	
	Asset& asset_file(const fs::path& path);
	
	void read(WtfNode* node);
	void write(WtfWriter* ctx) const;
	void validate() const;
	
	Asset* lookup_asset(AssetReference reference);
	
	bool weakly_equal(const Asset& rhs) const;
	
	virtual void for_each_attribute(AssetVisitorCallback callback) = 0;
	virtual void for_each_attribute(ConstAssetVisitorCallback callback) const = 0;
	virtual void read_attributes(const WtfNode* node) = 0;
	virtual void write_attributes(WtfWriter* ctx) const = 0;
	virtual void validate_attributes() const = 0;
	
private:
	friend AssetPack;
	friend AssetFile;
	
	Asset& add_child(std::unique_ptr<Asset> child);
	
	Asset* lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end);
	
	void connect_precedence_pointers();
	void disconnect_precedence_pointers();
	
	AssetForest& _forest;
	AssetPack& _pack;
	AssetFile& _file;
	Asset* _parent;
	AssetType _type;
	std::string _tag;
	std::vector<std::unique_ptr<Asset>> _children;
	Asset* _lower_precedence = nullptr;
	Asset* _higher_precedence = nullptr;
};

class AssetFile {
public:
	AssetFile(AssetForest& forest, AssetPack& pack, const fs::path& relative_path);
	AssetFile(const AssetFile&) = delete;
	AssetFile(AssetFile&&) = delete;
	AssetFile& operator=(const AssetFile&) = delete;
	AssetFile& operator=(AssetFile&&) = delete;
	
	Asset& root();
	
	void write() const;
	
	std::unique_ptr<InputStream> open_binary_file_for_reading(const FileReference& reference, fs::file_time_type* modified_time_dest = nullptr) const;
	FileReference write_text_file(const fs::path& path, const char* contents) const;
	FileReference write_binary_file(const fs::path& path, Buffer contents) const;
	FileReference write_binary_file(const fs::path& path, std::function<void(OutputStream&)> callback) const;
	FileReference extract_binary_file(const fs::path& path, Buffer prepend, FILE* src, s64 offset, s64 size) const;
	
	AssetFile* lower_precedence();
	AssetFile* higher_precedence();
	
private:
	friend Asset;
	friend AssetPack;
	
	void read();
	
	AssetForest& _forest;
	AssetPack& _pack;
	fs::path _relative_directory;
	std::string _file_name;
	std::unique_ptr<Asset> _root;
};

class AssetPack {
public:
	virtual ~AssetPack();
	
	const char* name() const;
	bool is_writeable() const;
	
	AssetFile& asset_file(fs::path relative_path);
	
	void write() const;
	
	AssetPack* lower_precedence();
	AssetPack* higher_precedence();
	
	GameInfo game_info;
	
protected:
	AssetPack(AssetForest& forest, std::string name, bool is_writeable);
	AssetPack(const AssetPack&) = delete;
	AssetPack(AssetPack&&) = delete;
	AssetPack& operator=(const AssetPack&) = delete;
	AssetPack& operator=(AssetPack&&) = delete;
	
	std::string read_text_file(const FileReference& reference) const;
	std::vector<u8> read_binary_file(const FileReference& reference) const;
	
	std::function<void()> _unlocker; // We can't call virtual functions from the destructor so we use a lambda.
	
private:
	friend AssetForest;
	friend AssetFile;
	friend Asset;
	
	void read();
	
	Asset* lookup_local_asset(const AssetReference& absolute_reference);
	
	virtual std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const = 0;
	virtual std::string read_text_file(const fs::path& path) const = 0;
	virtual std::vector<u8> read_binary_file(const fs::path& path) const = 0;
	virtual void write_text_file(const fs::path& path, const char* contents) const = 0;
	virtual void write_binary_file(const fs::path& path, std::function<void(OutputStream&)> callback) const = 0;
	virtual void extract_binary_file(const fs::path& relative_dest, Buffer prepend, FILE* src, s64 offset, s64 size) const = 0;
	virtual std::vector<fs::path> enumerate_asset_files() const = 0;
	virtual s32 check_lock() const;
	virtual void lock();
	
	AssetForest& _forest;
	std::vector<std::unique_ptr<AssetFile>> _asset_files;
	std::string _name;
	bool _is_writeable;
	AssetPack* _lower_precedence = nullptr;
	AssetPack* _higher_precedence = nullptr;
};

class AssetForest {
public:
	AssetForest() {}
	AssetForest(const AssetForest&) = delete;
	AssetForest(AssetForest&&) = delete;
	AssetForest& operator=(const AssetForest&) = delete;
	AssetForest& operator=(AssetForest&&) = delete;
	
	template <typename Pack, typename... ConstructorArgs>
	AssetPack& mount(ConstructorArgs... args) {
		AssetPack* pack = _packs.emplace_back(std::make_unique<Pack>(*this, args...)).get();
		if(pack->is_writeable()) {
			if(s32 pid = pack->check_lock()) {
				fprintf(stderr, "error: Another process (with PID %d) has locked this asset pack. This implies the process is still alive or has previously crashed. To bypass this error, delete the lock file in the asset pack directory.\n", pid);
				throw std::logic_error("asset pack locked");
			} else {
				pack->lock();
			}
		}
		if(_packs.size() >= 2) {
			AssetPack* lower_pack = _packs[_packs.size() - 2].get();
			lower_pack->_higher_precedence = pack;
			pack->_lower_precedence = lower_pack;
		}
		pack->read();
		return *pack;
	}
	
	Asset* lookup_asset(const AssetReference& absolute_reference);

private:
	std::vector<std::unique_ptr<AssetPack>> _packs;
};

class LooseAssetPack : public AssetPack {
public:
	LooseAssetPack(AssetForest& forest, std::string name, fs::path directory, bool is_writeable);
	
private:
	std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const override;
	std::string read_text_file(const fs::path& path) const override;
	std::vector<u8> read_binary_file(const fs::path& path) const override;
	void write_text_file(const fs::path& path, const char* contents) const override;
	void write_binary_file(const fs::path& path, std::function<void(OutputStream&)> callback) const override;
	void extract_binary_file(const fs::path& relative_dest, Buffer prepend, FILE* src, s64 offset, s64 size) const override;
	std::vector<fs::path> enumerate_asset_files() const override;
	s32 check_lock() const override;
	void lock() override;
	
	fs::path _directory;
};

#endif
