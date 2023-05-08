/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2023 chaoticgd

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef CORE_FILEIO_H
#define CORE_FILEIO_H

#include <core/util/basic_util.h>

// Opaque structure with platform dependent implementation.
struct _wrench_file_handle;
typedef struct _wrench_file_handle WrenchFileHandle;

enum _wrench_file_mode {
    // Opens a file with read access, fails if file does not exist.
    WRENCH_FILE_MODE_READ              = 1,
    // Opens a file with write access, deletes original file if it exists.
    WRENCH_FILE_MODE_WRITE             = 2,
    // Opens a file with write access, keeps original file if it exists and writes to the end of the file.
    WRENCH_FILE_MODE_WRITE_APPEND      = 3,
    // Opens a file with read and write access, keeps original file if it exists, fails otherwise.
    WRENCH_FILE_MODE_READ_WRITE_MODIFY = 4,
    // Opens a file with read and write access, deletes original file if it exists.
    WRENCH_FILE_MODE_READ_WRITE_NEW    = 5,
    // Opens a file with read and write access, keeps original file if it exists and writes to the end of the file.
    WRENCH_FILE_MODE_READ_WRITE_APPEND = 6
} typedef WrenchFileMode;

enum _wrench_file_origin {
    // Origin is the beginning of the file.
    WRENCH_FILE_ORIGIN_START = 1,
    // Origin is the position of the current file pointer..
    WRENCH_FILE_ORIGIN_CURRENT = 2,
    // Origin is the end of the file.
    WRENCH_FILE_ORIGIN_END = 3
} typedef WrenchFileOrigin;

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode);
size_t file_read(void* buffer, size_t size, WrenchFileHandle* file);
size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file);
int file_seek(WrenchFileHandle* file, s64 offset, WrenchFileOrigin origin);
s64 file_tell(WrenchFileHandle* file);
s64 file_size(WrenchFileHandle* file);
int file_flush(WrenchFileHandle* file);
int file_close(WrenchFileHandle* file);

#endif /* CORE_FILEIO_H */
