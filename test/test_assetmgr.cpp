/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2025 chaoticgd

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

#include <catch2/catch_amalgamated.hpp>
#include <assetmgr/asset_types.h>

TEST_CASE("Asset System", "[assetmgr]")
{
	// Setup an asset forest.
	AssetForest forest;
	AssetBank& bank_a = forest.mount<MemoryAssetBank>();
	AssetFile& file_a = bank_a.asset_file("file_a.asset");
	
	AssetBank& bank_b = forest.mount<MemoryAssetBank>();
	AssetFile& file_b = bank_b.asset_file("file_b.asset");
	
	// Create some assets in file A.
	CollectionAsset& collection = file_a.root().child<CollectionAsset>("collection");
	BinaryAsset& binary_a = collection.child<BinaryAsset>("binary_a");
	ReferenceAsset& reference = file_a.root().child<ReferenceAsset>("reference");
	reference.set_asset("collection.binary_a");
	ReferenceAsset& double_reference_1 = file_a.root().child<ReferenceAsset>("double_reference_1");
	double_reference_1.set_asset("double_reference_2");
	ReferenceAsset& double_reference_2 = file_a.root().child<ReferenceAsset>("double_reference_2");
	double_reference_2.set_asset("collection.binary_a");
	
	// Create a placeholder shadowing the reference.
	file_b.root().child<PlaceholderAsset>("reference");
	
	// Test link resolution.
	SECTION("Absolute link lookup.")
	{
		Asset& asset = forest.lookup_asset("collection.binary_a", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Relative link lookup.")
	{
		Asset& asset = forest.lookup_asset("Collection:binary_a", &collection);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Double reference works.")
	{
		Asset& asset = forest.lookup_asset("double_reference_1", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Reference shadowed by placeholder still works.")
	{
		Asset& asset = forest.lookup_asset("reference", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
}
