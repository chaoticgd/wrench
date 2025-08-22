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

#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char* message_ok = "";
static char error_buffer[128] = "";

const char* FILEIO_ERROR_CONTEXT_STRING = error_buffer;

// This file provides a standard implementation of the fileio API.

enum _last_unflushed_op
{
	_last_unflushed_op_none = 0,
	_last_unflushed_op_read = 1,
	_last_unflushed_op_write = 2
};

struct _wrench_file_handle
{
	FILE* file;
	int may_flush;
	int update_mode;
	enum _last_unflushed_op last_op;
};

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode)
{
	assert(filename);
	assert(mode);

	const char* mode_string = NULL;
	int update_mode = 0;
	int may_flush = 0;
	switch(mode) {
		case WRENCH_FILE_MODE_READ:
			mode_string = "rb";
			update_mode = 0;
			may_flush = 0;
			break;
		case WRENCH_FILE_MODE_WRITE:
			mode_string = "wb";
			update_mode = 0;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_WRITE_APPEND:
			mode_string = "ab";
			update_mode = 0;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_MODIFY:
			mode_string = "r+b";
			update_mode = 1;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_NEW:
			mode_string = "w+b";
			update_mode = 1;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_APPEND:
			mode_string = "a+b";
			update_mode = 1;
			may_flush = 1;
			break;
		default: abort();
	}

	WrenchFileHandle* file = malloc(sizeof(WrenchFileHandle));
	if(!file) {
		snprintf(error_buffer, sizeof(error_buffer), "malloc: %s", strerror(errno));
		return NULL;
	}
	
	file->file = fopen(filename, mode_string);
	if(!file->file) {
		free(file);
		snprintf(error_buffer, sizeof(error_buffer), "fopen: %s", strerror(errno));
		return NULL;
	}

	file->update_mode = update_mode;
	file->last_op = _last_unflushed_op_none;
	file->may_flush = may_flush;

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return file;
}

size_t file_read(void* buffer, size_t size, WrenchFileHandle* file)
{
	assert(file);

	if(size == 0) {
		snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
		return 0;
	}

	assert(buffer);
	
	if(file->last_op == _last_unflushed_op_write) {
		if(file_flush(file) != 0) {
			return 0;
		}
	}

	size_t val = fread(buffer, 1, size, file->file);

	if(file->update_mode) {
		file->last_op = _last_unflushed_op_read;
	}

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}

size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file)
{
	assert(file);

	if(size == 0) {
		snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
		return 0;
	}

	assert(buffer);
	
	if(file->last_op == _last_unflushed_op_read) {
		if(file_flush(file) != 0) {
			return 0;
		}
	}

	size_t val = fwrite(buffer, 1, size, file->file);

	if(file->update_mode) {
		file->last_op = _last_unflushed_op_write;
	}

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}

size_t file_read_string(char* str, size_t buffer_size, WrenchFileHandle* file)
{
	assert(file);

	if(buffer_size == 0) {
		snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
		return 0;
	}

	assert(str);
	
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

size_t file_write_string(const char* str, WrenchFileHandle* file)
{
	assert(str);
	assert(file);
	return file_write(str, strlen(str), file);
}

size_t file_vprintf(WrenchFileHandle* file, const char* format, va_list vlist)
{
	assert(file);
	
	int val = vfprintf(file->file, format, vlist);
	if(val < 0) {
		snprintf(error_buffer, sizeof(error_buffer), "vfprintf: %s", strerror(errno));
		return val;
	}
	
	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}

size_t file_printf(WrenchFileHandle* file, const char* format, ...)
{
	assert(file);
	
	va_list list;
	va_start(list, format);
	size_t val = file_vprintf(file, format, list);
	va_end(list);
	
	return val;
}

int file_seek(WrenchFileHandle* file, size_t offset, WrenchFileOrigin origin)
{
	assert(file);
	
	int _origin;
	
	switch(origin) {
		case WRENCH_FILE_ORIGIN_START: _origin = SEEK_SET; break;
		case WRENCH_FILE_ORIGIN_CURRENT: _origin = SEEK_CUR; break;
		case WRENCH_FILE_ORIGIN_END: _origin = SEEK_END; break;
		default: abort();
	}
	
	file->last_op = _last_unflushed_op_none;
	
	int val = fseek(file->file, offset, _origin);
	if(val != 0) {
		snprintf(error_buffer, sizeof(error_buffer), "fseek: %s", strerror(errno));
		return EOF;
	}
	
	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}

size_t file_tell(WrenchFileHandle* file)
{
	assert(file);
	
	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return ftell(file->file);
}

size_t file_size(WrenchFileHandle* file)
{
	assert(file);
	
	size_t offset = file_tell(file);
	if(offset < 0) {
		return EOF;
	}
	if(file_seek(file, 0, WRENCH_FILE_ORIGIN_END) == EOF) {
		return EOF;
	}
	size_t size_val = file_tell(file);
	if(file_seek(file, offset, WRENCH_FILE_ORIGIN_START) == EOF) {
		return EOF;
	}
	
	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return size_val;
}

int file_flush(WrenchFileHandle* file)
{
	assert(file);

	if(file->may_flush == 0 || file->update_mode != 0 || file->last_op != _last_unflushed_op_write) {
		snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
		return 0;
	}

	int val = fflush(file->file);
	if(val != 0) {
		snprintf(error_buffer, sizeof(error_buffer), "fflush: %s", strerror(errno));
		return EOF;
	}

	file->last_op = _last_unflushed_op_none;

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}

int file_close(WrenchFileHandle* file)
{
	assert(file);
	assert(file->file);

	int val = fclose(file->file);
	free(file);
	if(val != 0) {
		snprintf(error_buffer, sizeof(error_buffer), "fclose: %s", strerror(errno));
		return EOF;
	}

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}
