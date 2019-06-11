#include "../stream.h"

#include <cstring>

/*
	A container for data stored on disc using sliding window compression.
	Used to store textures, level data, etc. Not to be confused with the *.WAD
	files in the game's filesystem. Those contain multiple different segments,
	some of which may be WAD compressed.
*/
packed_struct(wad_header,
	char magic[3]; // "WAD"
	uint32_t total_size;
	uint8_t pad[9];
	uint8_t data[0 /* total_size - sizeof(wad_header) */];
)

// Check the magic bytes.
bool validate_wad(wad_header header);

// Throws stream_io_error, stream_format_error.
void decompress_wad(stream& dest, stream& src);
