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
	AssetReference absolute_reference() const;
	AssetReference reference_relative_to(Asset& asset) const;
	
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
	
	template <typename ChildAsset>
	ChildAsset& child(std::string tag) {
		for(std::unique_ptr<Asset>& child : _children) {
			if(child->type() == ChildAsset::ASSET_TYPE && child->tag() == tag) {
				return static_cast<ChildAsset&>(*child.get());
			}
		}
		return add_child<ChildAsset>(std::move(tag));
	}
	
	Asset* find_child(AssetType type, const std::string& tag);
	
	bool remove_child(Asset& asset);
	
	void read(WtfNode* node);
	void write(WtfWriter* ctx) const;
	void validate() const;
	
	Asset* lookup_asset(AssetReference reference);
	
	virtual void for_each_attribute(AssetVisitorCallback callback) = 0;
	virtual void for_each_attribute(ConstAssetVisitorCallback callback) const = 0;
	virtual void read_attributes(const WtfNode* node) = 0;
	virtual void write_attributes(WtfWriter* ctx) const = 0;
	virtual void validate_attributes() const = 0;
	
private:
	friend AssetPack;
	
	template <typename ChildAsset>
	ChildAsset& add_child(std::string tag) {
		std::unique_ptr<ChildAsset> pointer = std::make_unique<ChildAsset>(forest(), pack(), file(), this, std::move(tag));
		ChildAsset* asset = pointer.get();
		_children.emplace_back(std::move(pointer));
		return *asset;
	}
	
	Asset& add_child(std::unique_ptr<Asset> child);
	
	Asset* lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end);
	
	void connect_precedence_pointers();
	void disconnect_precedence_pointers();
	
	AssetForest& _forest;
	AssetFile& _file;
	AssetPack& _pack;
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
	
	FileReference write_text_file(const fs::path& path, const char* contents) const;
	FileReference write_binary_file(const fs::path& path, Buffer contents) const;
	
	AssetFile* lower_precedence();
	AssetFile* higher_precedence();
	
private:
	friend AssetPack;
	
	void read();
	
	AssetPack& _pack;
	fs::path _relative_directory;
	std::string _file_name;
	std::unique_ptr<Asset> _root;
};

class AssetPack {
public:
	virtual ~AssetPack() {}
	
	bool is_writeable() const;
	
	AssetFile& asset_file(fs::path relative_path);
	
	void write() const;
	
	AssetPack* lower_precedence();
	AssetPack* higher_precedence();
	
protected:
	AssetPack(AssetForest& forest, bool is_writeable);
	AssetPack(const AssetPack&) = delete;
	AssetPack(AssetPack&&) = delete;
	AssetPack& operator=(const AssetPack&) = delete;
	AssetPack& operator=(AssetPack&&) = delete;
	
	std::string read_text_file(const FileReference& reference) const;
	std::vector<u8> read_binary_file(const FileReference& reference) const;
	
private:
	friend AssetForest;
	friend AssetFile;
	friend Asset;
	
	void read_asset_files();
	
	Asset* lookup_local_asset(const AssetReference& absolute_reference);
	
	virtual std::string read_text_file(const fs::path& path) const = 0;
	virtual std::vector<u8> read_binary_file(const fs::path& path) const = 0;
	virtual void write_text_file(const fs::path& path, const char* contents) const = 0;
	virtual void write_binary_file(const fs::path& path, Buffer contents) const = 0;
	virtual std::vector<fs::path> enumerate_asset_files() const = 0;
	virtual FILE* open_asset_write_handle(const fs::path& path) const = 0;
	
	AssetForest& _forest;
	std::vector<std::unique_ptr<AssetFile>> _asset_files;
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
		if(_packs.size() >= 2) {
			AssetPack* lower_pack = _packs[_packs.size() - 2].get();
			lower_pack->_higher_precedence = pack;
			pack->_lower_precedence = lower_pack;
		}
		pack->read_asset_files();
		return *pack;
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
	void write_text_file(const fs::path& path, const char* contents) const override;
	void write_binary_file(const fs::path& path, Buffer contents) const override;
	std::vector<fs::path> enumerate_asset_files() const override;
	FILE* open_asset_write_handle(const fs::path& path) const override;
	
	fs::path _directory;
};

#endif