#ifndef FORMATS_BMP_H
#define FORMATS_BMP_H

#include "../stream.h"

struct bmp_file_header;
struct bmp_info_header;
struct bmp_colour_table_entry;

/*
	The BMP file format.
	Not actually used by the game, just useful to have around.
*/
packed_struct(bmp_file_header,
	char              magic[2]; // "BM"
	uint32_t          file_size;
	uint32_t          reserved;
	file_ptr<uint8_t> pixel_data;
)

packed_struct(bmp_info_header,
	uint32_t info_header_size; // 40
	int32_t  width;
	int32_t  height;
	int16_t  num_colour_planes; // Must be 1.
	int16_t  bits_per_pixel;
	uint32_t compression_method; // 0 = RGB
	uint32_t pixel_data_size;
	int32_t  horizontal_resolution; // Pixels per metre.
	int32_t  vertical_resolution; // Pixels per metre.
	uint32_t num_colours;
	uint32_t num_important_colours; // Usually zero.
)

packed_struct(bmp_colour_table_entry,
	uint8_t b;
	uint8_t g;
	uint8_t r;
	uint8_t pad;
)

#endif
