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

#include "fileio.h"
#include <stdlib.h>
#include <stdio.h>

enum _last_unflushed_op {
    _last_unflushed_op_none = 0,
    _last_unflushed_op_read = 1,
    _last_unflushed_op_write = 2
};

struct _wrench_file_handle {
    FILE* file;
    int update_mode;
    enum _last_unflushed_op last_op;
};

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode) {
    verify(mode != 0, "No mode was specified when opening a file.");
    verify(filename != (const char*) 0, "Filename is NULL.");

    char* mode_string = (char*) 0;
    int update_mode = 0;
    switch (mode)
    {
        case WRENCH_FILE_MODE_READ:
            mode_string = (char*) "rb";
            update_mode = 0;
            break;
        case WRENCH_FILE_MODE_WRITE:
            mode_string = (char*) "wb";
            update_mode = 0;
            break;
        case WRENCH_FILE_MODE_WRITE_APPEND:
            mode_string = (char*) "ab";
            update_mode = 0;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_MODIFY:
            mode_string = (char*) "r+b";
            update_mode = 0;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_NEW:
            mode_string = (char*) "w+b";
            update_mode = 0;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_APPEND:
            mode_string = (char*) "a+b";
            update_mode = 0;
            break;
        default:
            verify_not_reached("No valid file access mode was specified.");
            break;
    }

    WrenchFileHandle* file = (WrenchFileHandle*) malloc(sizeof(WrenchFileHandle));

    verify(file != (WrenchFileHandle*) 0, "Failed to allocate WrenchFileHandle.");

    file->file = fopen(filename, mode_string);

    verify(file->file != (FILE*) 0, "Failed to open file %s.", filename);

    file->update_mode = update_mode;
    file->last_op = _last_unflushed_op_none;

    return file;
}

size_t file_read(void* buffer, size_t size, WrenchFileHandle* file) {
    if (size == 0) {
        return 0;
    }

    if (file->last_op == _last_unflushed_op_write) {
        file_flush(file);
    }

    size_t val = fread(buffer, 1, size, file->file);

    if (file->update_mode) {
        file->last_op = _last_unflushed_op_read;
    }

    return val;
}

size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file) {
    if (size == 0) {
        return 0;
    }

    if (file->last_op == _last_unflushed_op_read) {
        file_flush(file);
    }

    size_t val = fwrite(buffer, 1, size, file->file);

    if (file->update_mode) {
        file->last_op = _last_unflushed_op_write;
    }

    return val;
}

int file_seek(WrenchFileHandle* file, s64 offset, WrenchFileOrigin origin) {
    int _origin;

    switch (origin) {
        case WRENCH_FILE_ORIGIN_START:
            _origin = SEEK_SET;
            break;
        case WRENCH_FILE_ORIGIN_CURRENT:
            _origin = SEEK_CUR;
            break;
        case WRENCH_FILE_ORIGIN_END:
            _origin = SEEK_END;
            break;
        default:
            verify_not_reached("Invalid origin specified for file seeking.");
            break;
    }

    file->last_op = _last_unflushed_op_none;

    return fseek(file->file, offset, _origin);
}

s64 file_tell(WrenchFileHandle* file) {
    return ftell(file->file);
}

s64 file_size(WrenchFileHandle* file) {
	s64 offset = file_tell(file);
	file_seek(file, 0, WRENCH_FILE_ORIGIN_END);
	s64 size_val = file_tell(file);
	file_seek(file, offset, WRENCH_FILE_ORIGIN_START);

    return size_val;
}

int file_flush(WrenchFileHandle* file) {
    file->last_op = _last_unflushed_op_none;

    return fflush(file->file);
}

int file_close(WrenchFileHandle* file) {
    int val = fclose(file->file);

    free(file);

    return val;
}
