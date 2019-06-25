#include "bmp.h"

#include <cstring>

bool validate_bmp(bmp_file_header header) {
	return std::memcmp(header.magic, "BM", 2) == 0;
}
