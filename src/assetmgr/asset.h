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

#include <any>
#include <memory>
#include <functional>

#include <core/wtf.h>
#include <core/util.h>
#include <core/filesystem.h>

class AssetType {
	template <typename T>
	friend AssetType asset_type();
public:
	AssetType() = default;
	bool operator==(const AssetType& rhs) const { return string == rhs.string; }
	bool operator!=(const AssetType& rhs) const { return string != rhs.string; }

	const char* string = nullptr;
private:
	AssetType(const char* string_) : string(string_) {}
};

#define NULL_ASSET_TYPE AssetType()

using AssetVisitorCallback = std::function<void(const char* key, std::any value, std::function<void(std::any)> setter)>;
using ConstAssetVisitorCallback = std::function<void(const char* key, std::any value)>;

struct AssetReferenceFragment {
	AssetType type;
	std::string tag;
};

struct AssetReference {
	bool is_relative;
	std::vector<AssetReferenceFragment> fragments;
};

AssetReference parse_asset_reference(const char* ptr);
std::string asset_reference_to_string(const AssetReference& reference);

struct FileReference {
	fs::path path;
};

class AssetForest;
class AssetPack;
class AssetFile;

class Asset {
public:
	virtual ~Asset() = default;
	Asset(const Asset&) = delete;
	Asset(Asset&&) = delete;
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
	AssetReference absolute_reference() const;
	
	template <typename Callback>
	void for_each_child(Callback callback) {
		for(std::unique_ptr<Asset>& child : _children) {
			callback(*child.get());
		}
	}
	
	template <typename Callback>
	void for_each_child(Callback callback) const {
		for(const std::unique_ptr<Asset>& child : _children) {
			callback(*child.get());
		}
	}
	
	template <typename ChildAsset, typename... ConstructorArgs>
	ChildAsset& create_child(ConstructorArgs... args) {
		return _children.emplace_back(std::make_unique<ChildAsset>(args...));
	}
	
	Asset& add_child(std::unique_ptr<Asset> child);
	
	void read(WtfNode* node);
	void write(WtfWriter* ctx) const;
	void validate() const;
	
	Asset* lookup_asset(AssetReference reference);
	
	virtual void for_each_attribute(AssetVisitorCallback callback);
	virtual void for_each_attribute(ConstAssetVisitorCallback callback) const;
	virtual void read_attributes(const WtfNode* node);
	virtual void write_attributes(WtfWriter* ctx) const;
	virtual void validate_attributes() const;
	
protected:
	Asset(AssetForest& forest, AssetPack& pack, AssetFile& file, Asset* parent, AssetType type);
	
private:
	friend AssetPack;
	
	Asset* lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end);
	
	AssetForest& _forest;
	AssetFile& _file;
	AssetPack& _pack;
	Asset* _parent;
	AssetType _type;
	std::string _tag;
	std::vector<std::unique_ptr<Asset>> _children;
};

class AssetFile {
public:
	AssetFile(AssetForest& forest, AssetPack& pack, const fs::path& relative_path);
	AssetFile(const AssetFile&) = delete;
	AssetFile(AssetFile&&) = delete;
	AssetFile& operator=(const AssetFile&) = delete;
	AssetFile& operator=(AssetFile&&) = delete;
	
	Asset& root();
	
	void write();
	
	std::string read_relative_text_file(const FileReference& file) const;
	std::vector<u8> read_relative_binary_file(const FileReference& file) const;
	
private:
	AssetPack& _pack;
	fs::path _relative_directory;
	std::string _file_name;
	std::unique_ptr<Asset> _root;
};

class AssetPack {
public:
	virtual ~AssetPack();
	
	bool is_writeable() const;
	
	AssetFile& create_asset_file(const fs::path& relative_path);
	
protected:
	AssetPack(AssetForest& forest, bool is_writeable);
	AssetPack(const AssetPack&) = delete;
	AssetPack(AssetPack&&) = delete;
	AssetPack& operator=(const AssetPack&) = delete;
	AssetPack& operator=(AssetPack&&) = delete;
	
private:
	friend AssetForest;
	friend AssetFile;
	friend Asset;
	
	Asset* lookup_local_asset(const AssetReference& absolute_reference);
	
	virtual std::vector<fs::path> enumerate_asset_files() const;
	virtual std::string read_text_file(const fs::path& path) const;
	virtual std::vector<u8> read_binary_file(const fs::path& path) const;
	virtual FILE* open_asset_write_handle(const fs::path& path) const;
	
	AssetForest& _forest;
	std::vector<std::unique_ptr<AssetFile>> _asset_files;
	bool _is_writeable;
};

class AssetForest {
public:
	AssetForest();
	AssetForest(const AssetForest&) = delete;
	AssetForest(AssetForest&&) = delete;
	AssetForest& operator=(const AssetForest&) = delete;
	AssetForest& operator=(AssetForest&&) = delete;
	
	template <typename Pack, typename... ConstructorArgs>
	Pack& mount(ConstructorArgs... args) {
		return _packs.emplace_back(std::make_unique<Pack>(*this, args...));
	}
	
	Asset* lookup_asset(const AssetReference& absolute_reference);

private:
	std::vector<std::unique_ptr<AssetPack>> _packs;
};

class LooseAssetPack : public AssetPack {
public:
	LooseAssetPack(AssetForest& forest, fs::path directory);
private:
	
	std::string read_text_file(const fs::path& path) const override;
	std::vector<u8> read_binary_file(const fs::path& path) const override;
	FILE* open_asset_write_handle(const fs::path& path) const override;
	
	fs::path _directory;
};

#endif
