# Textures

## PIF Files

These are image files used by the game to store various textures including skins and GUI textures. PIF files are structured like so:

- Header
- Palette
- Mips (highest to lowest)

The header is structured like so:

```
packed_struct(PifHeader,
	/* 0x00 */ char magic[4];
	/* 0x04 */ s32 file_size;
	/* 0x08 */ s32 width;
	/* 0x0c */ s32 height;
	/* 0x10 */ s32 format;
	/* 0x14 */ s32 clut_format;
	/* 0x18 */ s32 clut_order;
	/* 0x1c */ s32 mip_levels;
)
```

## Swizzling

The palettes are swizzled like so:

```
static u8 map_palette_index(u8 index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	return (((index & 16) >> 1) != (index & 8)) ? (index ^ 0b00011000) : index;
}
```

The pixel data may also be swizzled according to the GS's native format.

## Special Effects

In UYA and DL, alpha values above 0x80 can be used to enable the bloom effect. Additionally, for certain types of textures the alpha channel is used to determine reflectivity (e.g. the aquatos sewer pipes).
