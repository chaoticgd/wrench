#include <catch2/catch_amalgamated.hpp>

#include <assetmgr/asset_types.h>

TEST_CASE("Asset System", "[assetmgr]") {
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
	reference.set_asset("reference.binary_b");
	ReferenceAsset& double_reference_1 = file_a.root().child<ReferenceAsset>("double_reference_1");
	double_reference_1.set_asset("double_reference_2");
	ReferenceAsset& double_reference_2 = file_a.root().child<ReferenceAsset>("double_reference_2");
	double_reference_2.set_asset("collection.binary_a");
	
	// Create some assets in file B.
	PlaceholderAsset& shadowing = file_b.root().child<PlaceholderAsset>("reference");
	BinaryAsset& binary_b = shadowing.child<BinaryAsset>("binary_b");
	
	// Test link resolution.
	SECTION("Absolute link lookup.") {
		Asset& asset = forest.lookup_asset("collection.binary_a", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Relative link lookup.") {
		Asset& asset = forest.lookup_asset("Collection:binary_a", &collection);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Double reference works.") {
		Asset& asset = forest.lookup_asset("double_reference_1", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_a.absolute_link().to_string());
	}
	
	SECTION("Reference shadowed by placeholder still works.") {
		Asset& asset = forest.lookup_asset("reference", nullptr);
		REQUIRE(asset.absolute_link().to_string() == binary_b.absolute_link().to_string());
	}
}
