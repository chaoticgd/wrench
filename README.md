wrench
======

A very early set of modding tools for the Ratchet & Clank PS2 games.

Screenshot
===========

![A screenshot of the level editor](screenshots/editor.png)

Status
======

The project isn't really usable for any serious modding yet.

- WAD Compression
	- [x] Decompression
	- [ ] Recompression
- Level Importing
	- Mobies (entities)
		- [x] Unique ID
		- [x] Object class/type
		- [x] Positions
		- [ ] Other attributes
	- [ ] Terrain, etc.
- Level Exporting
	- [ ] Mobies
- Texture Format
	- [x] 2FIP to BMP
	- [ ] BMP to 2FIP

CLI Tools
=========

- wad: Decompress WAD segments.
- fip: Extract 2FIP textures to indexed BMP files.
- scan: Scan for game data segments on the disc.
