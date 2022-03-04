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

class AssetManager;
class AssetPack;

class Asset {
public:
	virtual ~Asset() = default;
	
	AssetManager& manager();
	const AssetManager& manager() const;
	AssetPack& pack();
	const AssetPack& pack() const;
	Asset* parent();
	const Asset* parent() const;
	AssetType type() const;
	const std::string& tag() const;
	const fs::path file_path() const;
	const fs::path& directory_path() const;
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
	
	void read(WtfNode* node);
	void write(WtfWriter* ctx) const;
	void validate() const;
	
	Asset* lookup_asset(AssetReference reference);
	
	std::string read_text_file(const FileReference& file) const;
	std::vector<u8> read_binary_file(const FileReference& file) const;
	
	virtual void for_each_attribute(AssetVisitorCallback callback);
	virtual void for_each_attribute(ConstAssetVisitorCallback callback) const;
	virtual void read_attributes(const WtfNode* node);
	virtual void write_attributes(WtfWriter* ctx) const;
	virtual void validate_attributes() const;
	
protected:
	Asset(AssetManager& manager, AssetPack& pack, Asset* parent, AssetType type);
	
private:
	friend AssetManager;
	
	Asset* lookup_local_asset(const AssetReferenceFragment* begin, const AssetReferenceFragment* end);
	
	AssetManager& _manager;
	AssetPack& _pack;
	Asset* _parent;
	AssetType _type;
	std::string _tag;
	fs::path _dir;
	std::string _file_name;
	std::vector<std::unique_ptr<Asset>> _children;
};

class AssetPack {
public:
	virtual ~AssetPack();

	Asset* root();
	bool is_writeable() const;

protected:
	AssetPack(AssetManager& manager, bool is_writeable);
	
private:
	friend Asset;
	
	virtual std::string read_text_file(const fs::path& path) const;
	virtual std::vector<u8> read_binary_file(const fs::path& path) const;
	virtual FILE* open_asset_write_handle(const fs::path& path) const;
	
	std::unique_ptr<Asset> _root;
	bool _is_writeable;
};

class LooseAssetPack : public AssetPack {
private:
	LooseAssetPack(AssetManager& manager, fs::path directory);
	
	std::string read_text_file(const fs::path& path) const override;
	std::vector<u8> read_binary_file(const fs::path& path) const override;
	FILE* open_asset_write_handle(const fs::path& path) const override;
	
	fs::path _directory;
};

class AssetManager {
public:
	AssetManager();
	
	template <typename Pack, typename... ConstructorArgs>
	void mount(ConstructorArgs... args) {
		_packs.emplace_back(std::make_unique<Pack>(*this, args...));
	}
	
	Asset* lookup_asset(AssetReference absolute_reference);

private:
	std::vector<std::unique_ptr<AssetPack>> _packs;
};

#endif
