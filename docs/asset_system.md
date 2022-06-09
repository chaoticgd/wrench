# Asset System

The asset system is a set of software components designed to allow for the distribution of mods.

## Overview

An **asset bank** is a collection of source assets that represent either a mod or an unpacked version of the base game. Each asset bank contains a gameinfo.txt file containing metadata, and a set of asset files that reference in turn reference data files. The **asset files** are [Wrench Text Format](wrench_text_format.md) files that each define a physical tree of assets. For example:

	thing.textures {
		Texture 0 {
			src: 'texture_0.png'
		}
		
		Texture 1 {
			src: 'texture_1.png'
		}
	}

Here we define 4 assets. The thing asset, which is a Placeholder asset since it is collapsed, meaning it is not defined with its own body, any only exists to contain its enclosed asset, the textures asset, which is a Collection asset since it's not collapsed but its type name is omitted. The remaining two assets are Texture assets.

Now, suppose there was a second asset file like so:

	thing.textures {
		Texture 1 {
			src: 'texture_1_final.png'
		}
		
		Texture 2 {
			src: 'texture_2.png'
		}
	}

If these asset files are then both packed by Wrench, the logical tree that will be constructed looks like this, where the asset files from mods take priority over those from the base game:

	thing.textures {
		Texture 0 {
			src: 'texture_0.png'
		}
		
		Texture 1 {
			src: 'texture_1_final.png'
		}
		
		Texture 2 {
			src: 'texture_2.png'
		}
	}

## Asset Types

The asset types are all documented in the [Asset Reference](asset_reference_latest.md).

## Special Attributes

### `deleted`

This attribute makes it appear that the object does not exist in the logical tree, which is useful if the base game defines an asset that you want to be removed in your mod. An example:

	textures {
		Texture 0 {
			deleted: true
		}
	}

Here, the packer will treat the 0 asset as if it doesn't exist, even if it's defined in the base game.

## Internals

The asset system is implemented as a static library compiled from code stored in `src/assetmgr`.

### Core Classes

The `AssetForest` class represents an instance of the asset system in memory, and owns all of its memory.

The `AssetBank` class represents an asset bank and owns a set of `AssetFile` instances.

The `AssetFile` class represents a single .asset file and owns a tree of `Asset` instances.

The `Asset` class represents an asset definition inside a .asset file. A seperate physical tree of asset instances exist for each asset file, and precedence pointers are maintained for each asset that link them to assets in equivalent positions from different trees.

### Code Generator

Each asset type is defined in `asset_schema.wtf` and a code generator, `asset_codgen.cpp` to generate a C++ file that defines a subclass of `Asset` for each asset type. A similar generator program, `asset_docgen.cpp` is used to generate the asset reference document.
