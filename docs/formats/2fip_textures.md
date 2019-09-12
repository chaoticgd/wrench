# 2FIP Textures

These are image files used by the game to store various types of textures including skins and GUI textures.

The format uses indexed colour, so it uses a colour palette/CLUT. An entry in that table looks like this:

	packed_struct(fip_palette_entry,
		uint8_t r;
		uint8_t g;
		uint8_t b;
		uint8_t a;
	)

Note that the maximum value for the alpha is 0x80. The header looks like this:

	packed_struct(fip_header,
		char              magic[4]; // "2FIP"
		uint8_t           unknown1[0x4];
		uint32_t          width;
		uint32_t          height;
		uint8_t           unknown2[0x10];
		fip_palette_entry palette[0x100];
	)

Immediately following the header is the image data, which has a size of `width * height` in bytes. Each byte is an index into the colour palette with the middle two bits swapped. A function that decodes this into an index is provided below:

	uint8_t decode_palette_index(uint8_t index) {
		// Swap middle two bits
		//  e.g. 00010000 becomes 00001000.
		int a = index & 8;
		int b = index & 16;
		if(a && !b) {
			index += 8;
		} else if(!a && b) {
			index -= 8;
		}
		return index;
	}
