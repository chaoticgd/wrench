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
#include <stdio.h>
#include <string.h>

static const char* _fileio_message_ok = "No errors occurred.";
static char _fileio_message_error_buffer[128];

const char* FILEIO_ERROR_CONTEXT_STRING = "No errors occurred.";

#define _fileio_verify(condition, retval, message, ...) \
	{ \
		if(!(condition)) { \
			sprintf(_fileio_message_error_buffer, message, ##__VA_ARGS__); \
			FILEIO_ERROR_CONTEXT_STRING = _fileio_message_error_buffer; \
			return retval; \
		} \
	}

#define _fileio_verify_not_reached(retval, message, ...) \
	{ \
		sprintf(_fileio_message_error_buffer, message, ##__VA_ARGS__); \
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_error_buffer; \
		return retval; \
	}

/*
 * This file provides a standard implementation of the fileio API.
 * This implementation uses only functionalities provided by the C standard.
 *
 * Platforms that only provide a minimal implementation of the C standard library may require
 * their own platform specific implementation.
 *
 * Line endings in this implementation are assumed to be '\n'.
 */

enum _last_unflushed_op { _last_unflushed_op_none = 0, _last_unflushed_op_read = 1, _last_unflushed_op_write = 2 };

struct _wrench_file_handle {
	FILE* file;
	int update_mode;
	enum _last_unflushed_op last_op;
};

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode) {
	_fileio_verify(mode != 0, (WrenchFileHandle*) 0, "No mode was specified when opening a file.");
	_fileio_verify(filename != (const char*) 0, (WrenchFileHandle*) 0, "Filename is NULL.");

	char* mode_string = (char*) 0;
	int update_mode = 0;
	switch(mode) {
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
		default: _fileio_verify_not_reached((WrenchFileHandle*) 0, "No valid file access mode was specified."); break;
	}

	WrenchFileHandle* file = (WrenchFileHandle*) malloc(sizeof(WrenchFileHandle));

	_fileio_verify(file != (WrenchFileHandle*) 0, (WrenchFileHandle*) 0, "Failed to allocate WrenchFileHandle.");

	file->file = fopen(filename, mode_string);

	_fileio_verify(file->file != (FILE*) 0, (WrenchFileHandle*) 0, "Failed to open file %s.", filename);

	file->update_mode = update_mode;
	file->last_op = _last_unflushed_op_none;

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return file;
}

size_t file_read(void* buffer, size_t size, WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(size == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	_fileio_verify(buffer != (void*) 0, 0, "Buffer was NULL.");

	if(file->last_op == _last_unflushed_op_write) {
		file_flush(file);
	}

	size_t val = fread(buffer, 1, size, file->file);

	if(file->update_mode) {
		file->last_op = _last_unflushed_op_read;
	}

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}

size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(size == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	_fileio_verify(buffer != (const void*) 0, 0, "Buffer was NULL.");

	if(file->last_op == _last_unflushed_op_read) {
		file_flush(file);
	}

	size_t val = fwrite(buffer, 1, size, file->file);

	if(file->update_mode) {
		file->last_op = _last_unflushed_op_write;
	}

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}

size_t file_read_string(char* str, size_t buffer_size, WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(buffer_size == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	_fileio_verify(str != (char*) 0, 0, "String buffer was NULL.");

	size_t num_bytes = file_read(str, buffer_size - 1, file);

	size_t offset = 0;
	for(size_t i = 0; i < num_bytes; i++) {
		if(str[i] == '\r') {
			str[i] = '\0';
			continue;
		}

		str[offset++] = str[i];
	}

	for(size_t i = offset; i < buffer_size; i++) {
		str[i] = '\0';
	}

	return offset;
}

size_t file_write_string(const char* str, WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");
	_fileio_verify(str != (const char*) 0, 0, "String buffer was NULL.");

	return file_write(str, strlen(str), file);
}

size_t file_vprintf(WrenchFileHandle* file, const char* format, va_list vlist) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	int val = vfprintf(file->file, format, vlist);

	_fileio_verify(val >= 0, 0, "Failed to write formatted string.");

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}

size_t file_printf(WrenchFileHandle* file, const char* format, ...) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	va_list list;
	va_start(list, format);
	size_t val = file_vprintf(file, format, list);
	va_end(list);

	return val;
}

int file_seek(WrenchFileHandle* file, size_t offset, WrenchFileOrigin origin) {
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");

	int _origin;

	switch(origin) {
		case WRENCH_FILE_ORIGIN_START: _origin = SEEK_SET; break;
		case WRENCH_FILE_ORIGIN_CURRENT: _origin = SEEK_CUR; break;
		case WRENCH_FILE_ORIGIN_END: _origin = SEEK_END; break;
		default: _fileio_verify_not_reached(EOF, "Invalid origin specified for file seeking."); break;
	}

	file->last_op = _last_unflushed_op_none;

	int val = fseek(file->file, offset, _origin);

	_fileio_verify(val == 0, EOF, "Failed to seek the file.");

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}

size_t file_tell(WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return ftell(file->file);
}

size_t file_size(WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	size_t offset = file_tell(file);
	file_seek(file, 0, WRENCH_FILE_ORIGIN_END);
	size_t size_val = file_tell(file);
	file_seek(file, offset, WRENCH_FILE_ORIGIN_START);

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return size_val;
}

int file_flush(WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");

	file->last_op = _last_unflushed_op_none;

	int val = fflush(file->file);

	_fileio_verify(val == 0, EOF, "Failed to flush the file.");

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}

int file_close(WrenchFileHandle* file) {
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");
	_fileio_verify(file->file != (FILE*) 0, EOF, "File handle is invalid.");

	int val = fclose(file->file);

	free(file);

	_fileio_verify(val == 0, EOF, "Failed to close the file.");

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return val;
}