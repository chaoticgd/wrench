Resource Paths
==============

These are strings present throughout wrench's GUI that point to the position of a specific data segment within the currently loaded ISO file. For example, `file(test.iso)+0x500` refers to an object at offset `0x500` within `test.iso`. Sometimes, a path may contain multiple plus symbols. For example, `file(test.iso)+0x500+0x20` usually means that there is a data segment at `0x500` and the target object is within that data segment `0x20` bytes later.

If an asset is compressed, the resource path will contain `wad(...)`. For example, `wad(file(test.iso)+0x500)+0x200` means that the target object is stored within a compressed segment starting at `0x500` within `test.iso` at an offset of `0x200` relative to the beginning of the decompressed data.
