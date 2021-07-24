# Format Specification

The source level format created for Wrench.

## Changelog

### Version 1

- Initial release.

You don't need to support version 1.

### Version 2

- Changed "index" to "id" for instances.
- Changed the existing "id" from the help messages to "string_id" and the existing id for the tie ambient RGBA values to "number".

You don't need to support version 2.

### Version 3

- Moved from storing each type of instance seperately to storing them all in a single gameplay.json file.
- Include format version and application version metadata in more files (the class files, the pvar type file).

You don't need to support version 3.

### Version 4

- For instances with matrices, the position is now stored in the 4th column of the matrix instead of under a seperate position key.
- The transformation information of sound instances is no longer nested under a "cuboid" key.
- Removed the redundant "id" field from the help messages, renamed the "string_id" field to just "id".
- Removed the redundant "number" field from the tie ambient RGBA values.
