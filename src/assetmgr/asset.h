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
// banks, which contain .asset files, which contain a tree of assets. Each mod
// as well as each base game here would be represented by an asset pack.

#include <memory>
#include <functional>

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>
#include <core/stream.h>
#include <core/memory_profiler.h>
#include <assetmgr/game_info.h>
#include <assetmgr/asset_util.h>
#include <assetmgr/asset_dispatch.h>

enum AssetFlags {
	ASSET_IS_WAD = (1 << 0),
	ASSET_IS_LEVEL_WAD = (1 << 1),
	ASSET_IS_BIN_LEAF = (1 << 2),
	ASSET_IS_FLATTENABLE = (1 << 3),
	ASSET_HAS_DELETED_FLAG = (1 << 4),
	ASSET_IS_DELETED = (1 << 5)
};

class Asset {
protected:
	Asset(AssetFile& file, Asset* parent, AssetType type, std::string tag, AssetDispatchTable& func_table);
public:
	Asset(const Asset&) = delete;
	Asset(Asset&&) = delete;
	virtual ~Asset();
	Asset& operator=(const Asset&) = delete;
	Asset& operator=(Asset&&) = delete;
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	AssetForest& forest();
	const AssetForest& forest() const;
	AssetBank& bank();
	const AssetBank& bank() const;
	AssetFile& file();
	const AssetFile& file() const;
	Asset* parent();
	const Asset* parent() const;
	const std::string& tag() const;
	Asset* lower_precedence();
	const Asset* lower_precedence() const;
	Asset* higher_precedence();
	const Asset* higher_precedence() const;
	Asset& lowest_precedence();
	const Asset& lowest_precedence() const;
	Asset& highest_precedence();
	const Asset& highest_precedence() const;
	AssetReference reference() const;
	
	// This might return Plaholder instead of the actual type.
	AssetType type() const;
	// This skips over placeholder nodes to get the logical type.
	AssetType logical_type() const;
	
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
		for(Asset* asset = &lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->_children) {
				if(child->higher_precedence() == nullptr && !child->is_deleted()) {
					callback(child->resolve_references());
				}
			}
		}
	}
	
	template <typename Callback>
	void for_each_logical_child(Callback callback) const {
		const_cast<Asset&>(*this).for_each_logical_child([&](const Asset& child) {
			callback(child);
		});
	}
	
	template <typename ChildType, typename Callback>
	void for_each_logical_child_of_type(Callback callback) {
		for(Asset* asset = &lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->_children) {
				if(child->higher_precedence() == nullptr && !child->is_deleted()) {
					Asset& child_2 = child->resolve_references();
					if(child_2.type() == ChildType::ASSET_TYPE) {
						callback(static_cast<ChildType&>(child_2));
					}
				}
			}
		}
	}
	
	template <typename ChildType, typename Callback>
	void for_each_logical_child_of_type(Callback callback) const {
		const_cast<Asset&>(*this).for_each_logical_child_of_type<ChildType>([&](const ChildType& child) {
			callback(child);
		});
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
		for(Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
			if(asset->type() == AssetType::ASSET_TYPE) {
				return *static_cast<AssetType*>(asset);
			}
		}
		verify_not_reached("Failed to convert asset %s to type %s.",
			asset_reference_to_string(reference()).c_str(),
			asset_type_to_string(AssetType::ASSET_TYPE));
	}
	
	template <typename AssetType>
	const AssetType& as() const {
		for(const Asset* asset = &highest_precedence(); asset != nullptr; asset = asset->lower_precedence()) {
			if(asset->type() == AssetType::ASSET_TYPE) {
				return *static_cast<const AssetType*>(asset);
			}
		}
		verify_not_reached("Failed to convert asset %s to type %s.",
			asset_reference_to_string(reference()).c_str(),
			asset_type_to_string(AssetType::ASSET_TYPE));
	}
	
	template <typename ChildTargetType>
	ChildTargetType& transmute_child(const char* tag) {
		for(auto iter = _children.begin(); iter != _children.end(); iter++) {
			if(iter->get()->tag() == tag) {
				_children.erase(iter);
				break;
			}
		}
		Asset& asset = add_child(create_asset(ChildTargetType::ASSET_TYPE, file(), this, tag));
		return asset.as<ChildTargetType>();
	}
	
	bool has_child(const char* tag) const;
	bool has_child(s32 tag) const;
	Asset& get_child(const char* tag);
	const Asset& get_child(const char* tag) const;
	Asset& get_child(s32 tag);
	const Asset& get_child(s32 tag) const;
	
	Asset& physical_child(AssetType type, const char* tag);
	Asset* get_physical_child(const char* tag);
	bool remove_physical_child(Asset& asset);
	
	// Switch to another .asset file, and create a child of the node in the same
	// place in the tree as the current node.
	template <typename ChildType>
	ChildType& foreign_child(std::string path, std::string tag) {
		return foreign_child_impl(path, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	template <typename ChildType>
	ChildType& foreign_child(std::string path, s32 index) {
		std::string tag = std::to_string(index);
		return foreign_child_impl(path, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	template <typename ChildType>
	ChildType& foreign_child(s32 index) {
		std::string tag = std::to_string(index);
		fs::path path = fs::path(tag)/tag;
		return foreign_child_impl(path, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	Asset& foreign_child_impl(const fs::path& path, AssetType type, const char* tag);
	
	void read(WtfNode* node);
	void write(WtfWriter* ctx, std::string prefix) const;
	void write_body(WtfWriter* ctx) const;
	void validate() const;
	
	bool weakly_equal(const Asset& rhs) const;
	
	void rename(std::string new_tag);
	
	bool is_deleted() const;
	
	virtual void for_each_attribute(AssetVisitorCallback callback) = 0;
	virtual void for_each_attribute(ConstAssetVisitorCallback callback) const = 0;
	virtual void read_attributes(const WtfNode* node) = 0;
	virtual void write_attributes(WtfWriter* ctx) const = 0;
	virtual void validate_attributes() const = 0;
	
	AssetDispatchTable& funcs;
	u16 flags = 0;
	
private:
	friend AssetBank;
	friend AssetFile;
	
	Asset& add_child(std::unique_ptr<Asset> child);
	
	Asset& resolve_references();
	
	void connect_precedence_pointers();
	void disconnect_precedence_pointers();
	
	AssetType _type;
	AssetFile& _file;
	Asset* _parent;
	std::string _tag;
	std::vector<std::unique_ptr<Asset>> _children;
	Asset* _lower_precedence = nullptr;
	Asset* _higher_precedence = nullptr;

protected:
	u32 _attrib_exists = 0;
};

class AssetFile {
public:
	AssetFile(AssetForest& forest, AssetBank& pack, const fs::path& relative_path);
	AssetFile(const AssetFile&) = delete;
	AssetFile(AssetFile&&) = delete;
	AssetFile& operator=(const AssetFile&) = delete;
	AssetFile& operator=(AssetFile&&) = delete;
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	Asset& root();
	
	void write() const;
	
	std::unique_ptr<InputStream> open_binary_file_for_reading(const FileReference& reference, fs::file_time_type* modified_time_dest = nullptr) const;
	std::pair<std::unique_ptr<OutputStream>, FileReference> open_binary_file_for_writing(const fs::path& path) const;
	std::string read_text_file(const fs::path& path) const;
	FileReference write_text_file(const fs::path& path, const char* contents) const;
	
	AssetFile* lower_precedence();
	AssetFile* higher_precedence();
	
private:
	friend Asset;
	friend AssetBank;
	
	void read();
	
	AssetForest& _forest;
	AssetBank& _bank;
	fs::path _relative_directory;
	std::string _file_name;
	std::unique_ptr<Asset> _root;
};

class AssetBank {
public:
	virtual ~AssetBank();
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	bool is_writeable() const;
	
	AssetFile& asset_file(fs::path path);
	
	void write();
	
	AssetBank* lower_precedence();
	AssetBank* higher_precedence();
	
	Asset* root();
	
	GameInfo game_info;
	
protected:
	AssetBank(AssetForest& forest, bool is_writeable);
	AssetBank(const AssetBank&) = delete;
	AssetBank(AssetBank&&) = delete;
	AssetBank& operator=(const AssetBank&) = delete;
	AssetBank& operator=(AssetBank&&) = delete;
	
	std::string read_text_file(const FileReference& reference) const;
	std::vector<u8> read_binary_file(const FileReference& reference) const;
	
	std::function<void()> _unlocker; // We can't call virtual functions from the destructor so we use a lambda.
	
private:
	friend AssetForest;
	friend AssetFile;
	friend Asset;
	
	void read();
	
	virtual std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const = 0;
	virtual std::unique_ptr<OutputStream> open_binary_file_for_writing(const fs::path& path) = 0;
	virtual std::string read_text_file(const fs::path& path) const = 0;
	virtual void write_text_file(const fs::path& path, const char* contents) = 0;
	virtual std::vector<fs::path> enumerate_asset_files() const = 0;
	virtual s32 check_lock() const;
	virtual void lock();
	
	AssetForest& _forest;
	std::vector<std::unique_ptr<AssetFile>> _asset_files;
	bool _is_writeable;
	AssetBank* _lower_precedence = nullptr;
	AssetBank* _higher_precedence = nullptr;
};

class AssetForest {
public:
	AssetForest() {}
	AssetForest(const AssetForest&) = delete;
	AssetForest(AssetForest&&) = delete;
	AssetForest& operator=(const AssetForest&) = delete;
	AssetForest& operator=(AssetForest&&) = delete;
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	Asset& lookup_asset(const AssetReference& reference, Asset* context);
	
	template <typename Pack, typename... ConstructorArgs>
	AssetBank& mount(ConstructorArgs... args) {
		AssetBank* bank = _banks.emplace_back(std::make_unique<Pack>(*this, args...)).get();
		if(bank->is_writeable()) {
			if(s32 pid = bank->check_lock()) {
				fprintf(stderr, "error: Another process (with PID %d) has locked this asset bank. This implies the process is still alive or has previously crashed. To bypass this error, delete the lock file in the asset bank directory.\n", pid);
				throw std::logic_error("asset bank locked");
			} else {
				bank->lock();
			}
		}
		if(_banks.size() >= 2) {
			AssetBank* lower_bank = _banks[_banks.size() - 2].get();
			lower_bank->_higher_precedence = bank;
			bank->_lower_precedence = lower_bank;
		}
		bank->read();
		return *bank;
	}
	
	void unmount_last();

private:
	std::vector<std::unique_ptr<AssetBank>> _banks;
};

class LooseAssetBank : public AssetBank {
public:
	LooseAssetBank(AssetForest& forest, fs::path directory, bool is_writeable);
	
private:
	std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const override;
	std::unique_ptr<OutputStream> open_binary_file_for_writing(const fs::path& path) override;
	std::string read_text_file(const fs::path& path) const override;
	void write_text_file(const fs::path& path, const char* contents) override;
	std::vector<fs::path> enumerate_asset_files() const override;
	s32 check_lock() const override;
	void lock() override;
	
	fs::path _directory;
};

class MemoryAssetBank : public AssetBank {
public:
	MemoryAssetBank(AssetForest& forest);
	
private:
	std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const override;
	std::unique_ptr<OutputStream> open_binary_file_for_writing(const fs::path& path) override;
	std::string read_text_file(const fs::path& path) const override;
	void write_text_file(const fs::path& path, const char* contents) override;
	std::vector<fs::path> enumerate_asset_files() const override;
	s32 check_lock() const override;
	void lock() override;
	
	std::map<fs::path, std::vector<u8>> _files;
};

#endif
