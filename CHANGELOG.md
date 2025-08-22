# Changelog

## v0.6

- Use glTF (.glb) to store moby models instead of COLLADA.

## v0.5

- Rewrote the save editor so that it now works for all the games.
- Use glTF (.glb) to store the sky models instead of COLLADA.
- Use glTF (.glb) to store the shrub models instead of COLLADA.
- Sky models are now split up into clusters automatically, making custom skies much more doable.
- Added a collision fixer tool in the editor to recover instanced collision for ties and shrubs semi-automatically.
- Added proper 3D transformation gizmos in the editor for translation, rotation and scaling.
- The SYSTEM.CNF file is now written out onto the correct sector for R&C1.

## v0.4

New features:
- Added a new instance system and format for storing and editing instances/entities/game objects.
- Added a special-purpose C++ parser for defining pvar data types.
- Added an overlay asset bank, mounted between the game and mods, for built-in pvar data types.
- Added an experimental memory card editor (mostly Deadlocked only).
- Occlusion data for a level can now be rebuilt on demand from the level editor.
- Packed ELF files and the level code overlays can now be unpacked to a regular ELF file.
- Tfrag meshes are now unpacked with quad faces (internally tfaces are recovered, but I'm yet to develop a solution to properly export that information).
- Overhauled the documentation.

Changes:
- Tfrag and collision assets are no longer duplicated in the unpacked asset files (they are now only stored inside Chunk assets, even in the case of R&C1).
- The underlay is no longer stored in its own zip file. This should make it more convenient to modify.

Bug fixes:
- Fixed some issues with unpacking certain builds.
- Improved handling of unicode in file paths on Windows.
- A lot more.

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
