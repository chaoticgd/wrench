Resource Paths
==============

These are strings present throughout wrench's GUI that point to the position of a specific data segment within the currently loaded ISO file. For example, `file(test.iso)+0x500` refers to an object at offset `0x500` within `test.iso`. Sometimes, a path may contain multiple plus symbols. For example, `file(test.iso)+0x500+0x20` usually means that there is a data segment at `0x500` and the target object is within that data segment `0x20` bytes later.

If an asset is compressed, the resource path will contain `wad(...)`. For example, `wad(file(test.iso)+0x500)+0x200` means that the target object is stored within a compressed segment starting at `0x500` within `test.iso` at an offset of `0x200` relative to the beginning of the decompressed data.

Most of the resource paths won't use the `file(...)` stream but will instead use an `iso` stream. This means that the source stream is a raw ISO file with a patch applied. The ISO file itself is read-only, and the patch is written to a seperate file when saved. For example, `iso+0x100` refers to an object at offset `0x100` within the currently loaded ISO with a given patch applied.
