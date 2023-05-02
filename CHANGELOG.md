# Changelog

## v0.4

New features:
- Added an experimental memory card editor (mostly Deadlocked only).
- Occlusion data for a level is now rebuilt during the packing process. This means that objects should no longer disappear when they are moved far from their initial position.
- Packed ELF files and the level code overlays can now be unpacked to a regular ELF file.
- Tfrag meshes are now unpacked with quad faces (internally tfaces are recovered, but I'm yet to develop a solution to properly export that information).
- The underlay is no longer stored in its own zip file. This should make it more convenient to modify.

Bug fixes:
- Fixed some issues with unpacking certain builds.

## v0.3

New features:
- Tfrag meshes can now be unpacked and are displayed in the editor.
- Tie meshes can now be unpacked and are displayed in the editor.
- Mods can now be loaded from zip files.
- An underlay asset bank is now included, which is used by the unpacker to give the game's files and folders more human-readable names on disk.
- Moby classes stored in missions are now unpacked for Deadlocked.
- Tooltips have been added to the launcher to fix some usability issues.

Changes:
- Merged the LevelDataWad and LevelCore asset types into the LevelWad asset type.

Bug fixes:
- Fixed a crash when packing shrub meshes containing an edge connecting 3 or more faces.
- Wrench should now function correctly when it is placed in a folder with a path that contains spaces on Windows.

## v0.2

New features:
- Shrub models can now be unpacked, repacked and are now displayed in the editor.
- Sky models can now be unpacked and repacked.
- A command line option has been added to unpack loose built collision files.
- Unpacking and repacking of stashed textures (textures that are always present in GS memory) is now supported.
- Individual moby bangles (small supplementary models that can be turned on or off like destructible parts of an object) are now unpacked separately.

Bug fixes:
- Repacked builds of UYA and Deadlocked will no longer all crash on level load.
- An issue where the gameplay file was written out incorrectly for games other than R&C2 has been fixed.
- The region command line argument of wrenchbuild is now parsed correctly.
- The SYSTEM.CNF file is now written out correctly.
- Texture swizzling is now performed in more places for Deadlocked.
- The "New Mod" screen will now populate more fields of the generated gameinfo.txt file correctly.
- Improved the COLLADA parser.

## v0.1

- The first proper prerelease!
