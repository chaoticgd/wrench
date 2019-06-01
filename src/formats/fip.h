#ifndef FORMATS_FIP_H
#define FORMATS_FIP_H

#include "../stream.h"

/*
	An uncompressed, indexed image format.
	Often stored compressed within WAD segments.
*/
struct fip_header;
struct fip_palette_entry;

packed_struct(fip_palette_entry,
	uint8_t r;
	uint8_t g;
	uint8_t b;
	uint8_t pad;
)

packed_struct(fip_header,
	char              magic[4]; // "2FIP"
	uint8_t           unknown1[0x4];
	uint32_t          width;
	uint32_t          height;
	uint8_t           unknown2[0x10];
	fip_palette_entry palette[0x100];
	uint8_t           data[0];
)

void fip_to_bmp(stream& dest, stream& src);

#endif
