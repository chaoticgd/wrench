# Asset System

The asset system is a set of software components designed to allow for the distribution of mods.

## Overview

An **asset bank** is a collection of source assets that represent either a mod or an unpacked version of the base game. Each asset bank contains a gameinfo.txt file containing metadata, and a set of .asset files that in turn reference data files. The **asset files** are [Wrench Text Format](wrench_text_format.md) files that each define a physical tree of assets. For example:

	thing.textures {
		Texture 0 {
			src: "texture_0.png"
		}
		
		Texture 1 {
			src: "texture_1.png"
		}
	}

Here we define 4 assets. The thing asset, which is a Placeholder asset since it is collapsed, meaning it is not defined with its own body, any only exists to contain its enclosed asset, the textures asset, which is a Collection asset since it's not collapsed but its type name is omitted. The remaining two assets are Texture assets.

Now, suppose there was a second asset file like so:

	thing.textures {
		Texture 1 {
			src: "texture_1_final.png"
		}
		
		Texture 2 {
			src: "texture_2.png"
		}
	}

If these asset files are then both packed by Wrench, the logical tree that will be constructed looks like this, where the asset files from mods take priority over those from the base game:

	thing.textures {
		Texture 0 {
			src: "texture_0.png"
		}
		
		Texture 1 {
			src: "texture_1_final.png"
		}
		
		Texture 2 {
			src: "texture_2.png"
		}
	}

## Design Considerations

### Files versus Assets

Assets can be referenced from other asset banks, files cannot. Mods do not rely on where the unpacked files are on the filesystem, just where their assets are in the logical asset tree.

This allows for the structure of the unpacked files to change arbitrarily without affecting mod compatibility. For example, we have given levels and object classes human-friendly names in the filesystem and sorted them into directories in a manor such that we can change the names without breaking any mods.

### Different Releases

It should be possible to build a mod designed for the US release of a given game on the EU or Japanese release, for example. To facilitate this, the asset format should remain the same between releases.

An exception to this is where the format of the assets themselves differ between releases. For example, the MPEG cutscenes have a different framerate depending on whether it's an NTSC or PAL release, and these files cannot be swapped. In this case, the asset system should be able to represent both the NTSC and PAL assets seperately. For example the Mpeg asset has a child named video_nstc and a seperate child named video_pal.

## Asset Links

Asset links are strings that can be used to reference an asset. These links are composed of an optional prefix ending with a colon, and a set of asset tags seperated by dots. The syntax is as follows:

	[<prefix>:]<tag 1>.<tag 2>.(...).<tag N>

For example, the following asset link points to the bolt crate moby from R&C2:

	gc.moby_classes.500

This is an absolute asset link, meaning there is no prefix.

This link references the sound bank for a level:

	LevelWad:data.sound_bank

Here, the link is relative to the first ancestor, starting from the asset that includes said asset link (or its parent), that has a type equal to the prefix.

## Asset Types

The asset types are all documented in the [Asset Reference](asset_reference.md).

### Hints

Hints are strings passed to the packers/unpackers that provide additional context. For example, the Texture asset packer can be passed a hint to determine whether the texture should be packed as a paletted PIF image or an RGBA image.

All the valid hint strings are documented in the asset reference.

For the top-level asset being packed, which is usually a build, the hint string can be passed in as a command line argument using the -h option.

## Special Attributes

### `strongly_deleted` (since version 15)

This attribute makes it appear that the asset it is applied to does not exist in the logical tree, which is useful if the base game defines an asset that you want to be removed in your mod. An example:

	textures {
		Texture 0 {
			strongly_deleted: true
		}
	}

Here, the packer will treat the 0 asset as if it doesn't exist, even if it's defined in the base game.

### `weakly_deleted` (since version 15)

Same as `strongly_deleted` except it will be assumed to be set to `false` (instead of unset) if it is omitted for a given asset. This means that only the `weakly_deleted` attributes of the highest precedence assets will be read.

## The Underlay

In addition to the game and loaded mods, an underlay asset bank is also typically loaded while packing and unpacking. This contains metadata for assets that is not stored on the disc. For example, names of levels and object classes. In the future this could also include data type definitions for pvars.

## The `src` Directory

The subdirectories of the `src` directory are special. Instead of their files being referenced from `.asset` files, they are treated as an overlay or union filesystem e.g. if a lower precedence asset bank X contains `a.h` and `b.h`, and a higher precedence bank Y contains `b.h` and `c.h`, the `a.h` file from bank X will be used, and the `b.h` and `c.h` files from bank Y will be used. The following directories are treated like this:

- `src/game_common`: Always loaded.
- `src/game_rac`: Only loaded for R&C1.
- `src/game_gc`: Only loaded for GC.
- `src/game_uya`: Only loaded for UYA.
- `src/game_dl`: Only loaded for Deadlocked.

## Internals

The asset system is implemented as a static library compiled from code stored in `src/assetmgr`.

### Core Classes

The `AssetForest` class represents an instance of the asset system in memory, and owns all of its memory.

The `AssetBank` class represents an asset bank and owns a set of `AssetFile` instances.

The `AssetFile` class represents a single .asset file and owns a tree of `Asset` instances.

The `Asset` class represents an asset definition inside a .asset file. A seperate physical tree of asset instances exist for each asset file, and precedence pointers are maintained for each asset that link them to assets in equivalent positions from different trees.

### Code Generator

Each asset type is defined in `asset_schema.wtf` and a code generator, `asset_codegen.cpp` is used to generate a C++ file that defines a subclass of `Asset` for each asset type. A similar generator program, `asset_docgen.cpp` is used to generate the asset reference document.

## Version History

| Format Version | Wrench Version | Description |
| -    | -     | - |
| 29   |       | Added hero_groups attributes to Collision assets. |
| 28   |       | Use glTF (.glb) for the moby models instead of COLLADA. |
| 27   | v0.5  | Use glTF (.glb) for the shrub and sky models instead of COLLADA. The mesh attribute of the SkyShell asset is now of type Mesh instead of Collection. The starting_rotation and angular_velocity attributes now only will only apply for UYA and DL (which is more correct). |
| 26   |       | Added static_collision children to MobyClass, TieClass and ShrubClass assets. The corresponding instance types now have has_static_collision flags. |
| 25   | v0.4  | Added version_info nodes to the instances files containing information about the application writing out the file, and the format version. |
| 24   |       | Instance IDs are now rewritten to indices while the gameplay/mission files are packed. Added core child to mission instances asset. All colours stored on instances are now vec3s. |
| 23   |       | Added new relative_pvar_pointers attribute to instances that have a pvar component. |
| 22   |       | Occlusion is now computed on demand instead of during a build. Occlusion assets now have grid and mapping attributes. |
| 21   |       | Added new instances format. Added Instances asset type. Added the C++ parsing system and the overlay asset bank. |
| 20   |       | Added Occlusion asset type. The code child of LevelWad asset has been renamed to overlay. Removed the tfrags and collision children of the LevelWad asset (the assets from the first chunk are used instead). |
| 19   |       | Added ElfFile asset type. |
| 18   | v0.3  | Added editor_mesh attribute to the TieClass asset type. Material assets are now used to store tie textures. |
| 17   |       | Added Tfrags and TfragsCore asset types. Material assets are now used to store tfrag textures. |
| 16   |       | The billboard asset is now a child of ShrubClass assets instead of ShrubClassCore assets. The mipmap_distance attribute of ShrubClassCore assets has been renamed to mip_distance. |d
| 15   |       | Added underlay asset banks. Level, MobyClass, TieClass and ShrubClass asset now have name and category attributes. Replaced deleted attribute with strongly_deleted and weakly_deleted attributes. |
| 14   |       | Moby classes stored in missions are now packed/unpacked separately for Deadlocked. |
| 13   |       | Merged LevelDataWad and LevelCore into the Level asset type. |
| 12   | v0.2  | Clank's textures are now packed/unpacked correctly for UYA. Modified how the rotation of sky shells is stored. |
| 11   |       | Shrub meshes are now stored as COLLADA files. ShrubClass/ShrubClassCore/Material overhauled. |
| 10   |       | Added stash_textures attribute to MobyClass asset. |
| 9    |       | Added Sky and SkyShell asset types. |
| 8    | v0.1  | The first version of the asset system. |
| 1..7 |       | Old JSON level format. Not supported. |
