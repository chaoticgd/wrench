# Format Specification

The source level format created for Wrench. Version 4 and up is currently supported.

## Changelog

### Version 1

- Initial release.

### Version 2

- Changed "index" to "id" for instances.
- Changed the existing "id" from the help messages to "string_id" and the existing id for the tie ambient RGBA values to "number".

### Version 3

- Moved from storing each type of instance seperately to storing them all in a single gameplay.json file.
- Include format version and application version metadata in more files (the class files, the pvar type file).

### Version 4

- For instances with matrices, the position is now stored in the 4th column of the matrix instead of under a seperate position key.
- The transformation information of sound instances is no longer nested under a "cuboid" key.
- Removed the redundant "id" field from the help messages, renamed the "string_id" field to just "id".
- Removed the redundant "number" field from the tie ambient RGBA values.
- The moby instances list is no longer nested under a "static_instances" key. The dynamic moby count is now called "dynamic_moby_count" in the top-level object.
- Bounding spheres are now stored as regular 4D vectors, so the "radius" key is now "w".
- Renamed moby instance fields that were in the form "rac23.unknown_%d" to "rac23_unknown_%d".
- Renamed light trigger field "light_index" to just "light" (since it's an ID).

### Version 5

- Store the global pvar, gc_88_dl_6c and the parts of gc_80_dl_64 as a JSON array of strings storing hex-encoded data, instead of as a JSON array of 8-bit unsigned integers.

### Version 6

- Added support for the first Ratchet & Clank game.
	- Added rac1_78 as a top-level JSON hex buffer in the gameplay file.
	- Added RAC1_7c instance type.
	- Added RAC1_88 instance type.
	- Addded moby fields: rac1_unknown_4, rac1_unknown_8, rac1_unknown_c, rac1_unknown_10, rac1_unknown_14, rac1_unknown_18, rac1_unknown_1c, rac1_unknown_20, rac1_unknown_24, rac1_unknown_28, rac1_unknown_2c, rac1_unknown_48, rac1_unknown_4c, rac1_unknown_50, rac1_unknown_54, rac1_unknown_5c, rac1_unknown_60, rac1_unknown_64, rac1_unknown_68, rac1_unknown_6c, rac1_unknown_70, rac1_unknown_74.
- Improved how the first part of the properties gameplay block is stored:
	- unknown_4c, unknown_50 and unknown_54 are now "ship_colour".
	- unknown_58 was removed (it is now treated as padding).
