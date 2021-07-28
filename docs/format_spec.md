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
- Bounding spheres are now stored as regular 4D vectors, so the "redius" key is now "w".
- Renamed moby instance fields that were in the form "rac23.unknown_%d" to "rac23_unknown_%d".
- Renamed light trigger field "light_index" to just "light" (since it's an ID).

## Version 5

- Store the global pvar, gc_88_dl_6c and the parts of gc_80_dl_64 as a JSON array of strings storing hex-encoded data, instead of as a JSON array of 8-bit unsigned integers.
