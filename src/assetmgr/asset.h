/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

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
// as well as each base game here would be represented by an asset bank.

#include <map>
#include <memory>
#include <functional>

#include <wtf/wtf.h>
#include <wtf/wtf_writer.h>
#include <core/stream.h>
#include <core/memory_profiler.h>
#include <cppparser/cpp_parser.h>
#include <assetmgr/game_info.h>
#include <assetmgr/asset_util.h>
#include <assetmgr/asset_dispatch.h>

enum AssetFlags
{
	ASSET_IS_WAD = (1 << 0),                    // This asset is a WAD file.
	ASSET_IS_LEVEL_WAD = (1 << 1),              // This asset is a level WAD file.
	ASSET_IS_BIN_LEAF = (1 << 2),               // This makes unpack_binaries dump this out excluding children.
	ASSET_IS_FLATTENABLE = (1 << 3),            // This asset can be written as a FlatWadAsset for debugging.
	ASSET_HAS_STRONGLY_DELETED_FLAG = (1 << 4), // The strongly deleted attribute was specified for this asset.
	ASSET_IS_STRONGLY_DELETED = (1 << 5),       // If the strongly deleted attribute was specified, it was set to true.
	ASSET_IS_WEAKLY_DELETED = (1 << 6)          // Like the strongly deleted flag, but is treated as false if not set.
};

class Asset
{
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
	
	AssetLink absolute_link() const;
	AssetLink link_relative_to(Asset& base) const;
	
	// This might return Placeholder instead of the type you probably want.
	AssetType physical_type() const;
	
	// This skips over placeholder nodes to get the logical type.
	AssetType logical_type() const;
	
	template <typename Callback>
	void for_each_physical_child(Callback callback)
	{
		for(std::unique_ptr<Asset>& child : m_children) {
			callback(*child.get());
		}
	}
	
	
	template <typename Callback>
	void for_each_physical_child(Callback callback) const
	{
		for(const std::unique_ptr<Asset>& child : m_children) {
			callback(*child.get());
		}
	}
	
	template <typename Callback>
	void for_each_logical_child(Callback callback)
	{
		for(Asset* asset = &lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->m_children) {
				if(child->higher_precedence() == nullptr && !child->is_deleted()) {
					callback(child->resolve_references());
				}
			}
		}
	}
	
	template <typename Callback>
	void for_each_logical_child(Callback callback) const
	{
		const_cast<Asset&>(*this).for_each_logical_child([&](const Asset& child) {
			callback(child);
		});
	}
	
	template <typename ChildType, typename Callback>
	void for_each_logical_child_of_type(Callback callback)
	{
		for(Asset* asset = &lowest_precedence(); asset != nullptr; asset = asset->higher_precedence()) {
			for(const std::unique_ptr<Asset>& child : asset->m_children) {
				if(child->higher_precedence() == nullptr && !child->is_deleted()) {
					Asset& child_2 = child->resolve_references();
					if(child_2.logical_type() == ChildType::ASSET_TYPE) {
						callback(child_2.as<ChildType>());
					}
				}
			}
		}
	}
	
	template <typename ChildType, typename Callback>
	void for_each_logical_child_of_type(Callback callback) const
	{
		const_cast<Asset&>(*this).for_each_logical_child_of_type<ChildType>([&](const ChildType& child) {
			callback(child);
		});
	}
	
	template <typename Callback>
	void for_each_logical_descendant(Callback callback)
	{
		for_each_logical_child([&](Asset& child) {
			callback(child);
			child.for_each_logical_descendant(callback);
		});
	}
	
	template <typename ChildType>
	ChildType& child(const char* tag)
	{
		Asset& asset = physical_child(ChildType::ASSET_TYPE, tag);
		ChildType& resolved_asset = asset.as<ChildType>();
		verify(&resolved_asset.file() == &file(),
			"Tried to access asset '%s' which is of the wrong type.",
			asset.absolute_link().to_string().c_str());
		return resolved_asset;
	}
	
	template <typename ChildType>
	ChildType& child(s32 tag)
	{
		std::string str = std::to_string(tag);
		Asset& asset = physical_child(ChildType::ASSET_TYPE, str.c_str());
		ChildType& resolved_asset = asset.as<ChildType>();
		verify(&resolved_asset.file() == &file(),
			"Tried to access asset '%s' which is of the wrong type.",
			asset.absolute_link().to_string().c_str());
		return resolved_asset;
	}
	
	Asset& as(AssetType type);
	Asset* maybe_as(AssetType type);
	
	template <typename AssetType>
	AssetType& as()
	{
		return static_cast<AssetType&>(as(AssetType::ASSET_TYPE));
	}
	
	template <typename AssetType>
	const AssetType& as() const
	{
		return const_cast<Asset*>(this)->as<AssetType>();
	}
	
	template <typename AssetType>
	AssetType* maybe_as()
	{
		return static_cast<AssetType*>(maybe_as(AssetType::ASSET_TYPE));
	}
	
	template <typename AssetType>
	const AssetType* maybe_as() const
	{
		return const_cast<Asset*>(this)->maybe_as<AssetType>();
	}
	
	template <typename ChildTargetType>
	ChildTargetType& transmute_child(const char* tag)
	{
		for(auto iter = m_children.begin(); iter != m_children.end(); iter++) {
			if(iter->get()->tag() == tag) {
				m_children.erase(iter);
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
	ChildType& foreign_child(std::string path, bool is_absolute, std::string tag)
	{
		return foreign_child_impl(path, is_absolute, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	template <typename ChildType>
	ChildType& foreign_child(std::string path, bool is_absolute, s32 index)
	{
		std::string tag = std::to_string(index);
		return foreign_child_impl(path, is_absolute, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	template <typename ChildType>
	ChildType& foreign_child(s32 index)
	{
		std::string tag = std::to_string(index);
		fs::path path = fs::path(tag)/tag;
		return foreign_child_impl(path, false, ChildType::ASSET_TYPE, tag.c_str()).template as<ChildType>();
	}
	
	Asset& foreign_child_impl(const fs::path& path, bool is_absolute, AssetType type, const char* tag);
	
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
	
	AssetType m_type;
	AssetFile& m_file;
	Asset* m_parent;
	std::string m_tag;
	std::vector<std::unique_ptr<Asset>> m_children;
	Asset* m_lower_precedence = nullptr;
	Asset* m_higher_precedence = nullptr;

protected:
	u32 m_attrib_exists = 0;
};

class AssetFile
{
public:
	AssetFile(AssetForest& forest, AssetBank& pack, const fs::path& relative_path);
	AssetFile(const AssetFile&) = delete;
	AssetFile(AssetFile&&) = delete;
	AssetFile& operator=(const AssetFile&) = delete;
	AssetFile& operator=(AssetFile&&) = delete;
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	Asset& root();
	std::string path() const;
	
	void write() const;
	
	std::pair<std::unique_ptr<OutputStream>, FileReference> open_binary_file_for_writing(const fs::path& path) const;
	FileReference write_text_file(const fs::path& path, const char* contents) const;
	bool file_exists(const fs::path& path) const;
	
	AssetFile* lower_precedence();
	AssetFile* higher_precedence();
	
	Asset& asset_from_link(AssetType type, const AssetLink& link);
	
private:
	friend Asset;
	friend AssetBank;
	friend FileReference;
	
	void read();
	
	std::unique_ptr<InputStream> open_binary_file_for_reading(const FileReference& reference, fs::file_time_type* modified_time_dest = nullptr) const;
	std::string read_text_file(const fs::path& path) const;
	
	AssetForest& m_forest;
	AssetBank& m_bank;
	fs::path m_relative_directory;
	std::string m_file_name;
	std::unique_ptr<Asset> m_root;
};

class AssetBank
{
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
	s32 index;
	
protected:
	AssetBank(AssetForest& forest, bool is_writeable);
	AssetBank(const AssetBank&) = delete;
	AssetBank(AssetBank&&) = delete;
	AssetBank& operator=(const AssetBank&) = delete;
	AssetBank& operator=(AssetBank&&) = delete;
	
	std::string read_text_file(const FileReference& reference) const;
	std::vector<u8> read_binary_file(const FileReference& reference) const;
	
	std::string get_common_source_path() const;
	std::string get_game_source_path(Game game) const;
	
	std::function<void()> m_unlocker; // We can't call virtual functions from the destructor so we use a lambda.
	
private:
	friend AssetForest;
	friend AssetFile;
	friend Asset;
	
	void read();
	
	virtual std::unique_ptr<InputStream> open_binary_file_for_reading(const fs::path& path, fs::file_time_type* modified_time_dest) const = 0;
	virtual std::unique_ptr<OutputStream> open_binary_file_for_writing(const fs::path& path) = 0;
	virtual std::string read_text_file(const fs::path& path) const = 0;
	virtual void write_text_file(const fs::path& path, const char* contents) = 0;
	virtual bool file_exists(const fs::path& path) const = 0;
	virtual std::vector<fs::path> enumerate_asset_files() const = 0;
	virtual void enumerate_source_files(std::map<fs::path, const AssetBank*>& dest, Game game) const = 0;
	virtual s32 check_lock() const;
	virtual void lock();
	
	AssetForest& m_forest;
	std::vector<std::unique_ptr<AssetFile>> m_asset_files;
	bool m_is_writeable;
	AssetBank* m_lower_precedence = nullptr;
	AssetBank* m_higher_precedence = nullptr;
};

class AssetForest
{
public:
	AssetForest() {}
	AssetForest(const AssetForest&) = delete;
	AssetForest(AssetForest&&) = delete;
	AssetForest& operator=(const AssetForest&) = delete;
	AssetForest& operator=(AssetForest&&) = delete;
	
	SETUP_MEMORY_ZONE(MEMORY_ZONE_ASSET_SYSTEM)
	
	Asset* any_root();
	const Asset* any_root() const;
	
	Asset& lookup_asset(const AssetLink& link, Asset* context);
	
	template <typename Bank, typename... ConstructorArgs>
	AssetBank& mount(ConstructorArgs... args)
	{
		AssetBank* bank = m_banks.emplace_back(std::make_unique<Bank>(*this, args...)).get();
		bank->index = (s32) (m_banks.size() - 1);
		if(bank->is_writeable()) {
			if(s32 pid = bank->check_lock()) {
				fprintf(stderr, "error: Another process (with PID %d) has locked this asset bank. This implies the process is still alive or has previously crashed. To bypass this error, delete the lock file in the asset bank directory.\n", pid);
				throw std::logic_error("asset bank locked");
			} else {
				bank->lock();
			}
		}
		if(m_banks.size() >= 2) {
			AssetBank* lower_bank = m_banks[m_banks.size() - 2].get();
			lower_bank->m_higher_precedence = bank;
			bank->m_lower_precedence = lower_bank;
		}
		bank->read();
		return *bank;
	}
	
	void unmount_last();
	void read_source_files(Game game);
	void write_source_files(AssetBank& bank, Game game);
	
	std::map<std::string, CppType>& types();
	const std::map<std::string, CppType>& types() const;
	
private:
	std::vector<std::unique_ptr<AssetBank>> m_banks;
	std::map<std::string, CppType> m_types;
};

class LooseAssetBank : public AssetBank
{
	friend AssetBank;
public:
	LooseAssetBank(AssetForest& forest, fs::path directory, bool is_writeable);
	
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
	public:
	fs::path m_directory;
};

class MemoryAssetBank : public AssetBank
{
public:
	MemoryAssetBank(AssetForest& forest);
	
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
	
	std::map<fs::path, std::vector<u8>> m_files;
};

#endif
