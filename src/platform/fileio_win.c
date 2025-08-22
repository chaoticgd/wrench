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

#include <windows.h>
#include <fileapi.h>

static const char* _fileio_message_ok = "";
static char _fileio_message_error_buffer[128];

const char* FILEIO_ERROR_CONTEXT_STRING = "";

struct _wrench_file_handle
{
	HANDLE file;
	int may_flush;
};

#define _fileio_verify_not_reached(retval, message, ...) \
	{ \
		sprintf(_fileio_message_error_buffer, message, ##__VA_ARGS__); \
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_error_buffer; \
		return retval; \
	}

#define _fileio_verify(condition, retval, message, ...) \
	{ \
		if(!(condition)) { \
			_fileio_verify_not_reached(retval, message, ##__VA_ARGS__); \
		} \
	}

#define _fileio_verify_with_clear(condition, retval, clear_command, message, ...) \
	{ \
		if(!(condition)) { \
			clear_command; \
			_fileio_verify_not_reached(retval, message, ##__VA_ARGS__); \
		} \
	}

struct _file_open_clear_list
{
	LPWSTR wide_string;
};

void _file_open_clear(struct _file_open_clear_list* list)
{
	if(list->wide_string != NULL)
		free(list->wide_string);
}

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode)
{
	_fileio_verify(mode != 0, (WrenchFileHandle*) 0, "No mode was specified when opening a file.");
	_fileio_verify(filename != (const char*) 0, (WrenchFileHandle*) 0, "Filename is NULL.");

	DWORD desired_access = 0;
	DWORD creation_disposition = 0;
	int may_flush = 0;

	switch(mode) {
		case WRENCH_FILE_MODE_READ:
			desired_access = FILE_GENERIC_READ;
			creation_disposition = OPEN_EXISTING;
			may_flush = 0;
			break;
		case WRENCH_FILE_MODE_WRITE:
			desired_access = FILE_GENERIC_WRITE;
			creation_disposition = CREATE_ALWAYS;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_WRITE_APPEND:
			desired_access = FILE_APPEND_DATA;
			creation_disposition = OPEN_ALWAYS;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_MODIFY:
			desired_access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
			creation_disposition = OPEN_EXISTING;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_NEW:
			desired_access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
			creation_disposition = CREATE_ALWAYS;
			may_flush = 1;
			break;
		case WRENCH_FILE_MODE_READ_WRITE_APPEND:
			desired_access = FILE_GENERIC_READ | FILE_APPEND_DATA;
			creation_disposition = OPEN_ALWAYS;
			may_flush = 1;
			break;
		default: _fileio_verify_not_reached((WrenchFileHandle*) 0, "No valid file access mode was specified."); break;
	}

	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	LPCCH input_string = (LPCCH) filename;

	int wide_string_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, (LPWSTR) 0, 0);
	_fileio_verify(wide_string_size != 0,
		(WrenchFileHandle*) 0,
		"MultiByteToWideChar: %lu.",
		GetLastError());

	struct _file_open_clear_list list = {.wide_string = NULL};

	list.wide_string = (LPWSTR) malloc(wide_string_size * sizeof(WCHAR));
	_fileio_verify_with_clear(list.wide_string != (LPWSTR) 0,
		(WrenchFileHandle*) 0,
		_file_open_clear(&list),
		"malloc");

	int written_bytes =
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, list.wide_string, wide_string_size);
	_fileio_verify_with_clear(written_bytes != 0,
		(WrenchFileHandle*) 0,
		_file_open_clear(&list),
		"MultiByteToWideChar: %lu.",
		GetLastError());

	HANDLE file_handle = CreateFileW(list.wide_string,
		desired_access,
		share_mode,
		(LPSECURITY_ATTRIBUTES) 0,
		creation_disposition,
		FILE_ATTRIBUTE_NORMAL,
		(HANDLE) 0);
	_fileio_verify_with_clear(file_handle != INVALID_HANDLE_VALUE,
		(WrenchFileHandle*) 0,
		_file_open_clear(&list),
		"CreateFileW: %lu.",
		GetLastError());

	WrenchFileHandle* file = (WrenchFileHandle*) malloc(sizeof(WrenchFileHandle));
	_fileio_verify_with_clear(file != (WrenchFileHandle*) 0,
		(WrenchFileHandle*) 0,
		_file_open_clear(&list),
		"malloc");

	file->file = file_handle;
	file->may_flush = may_flush;

	_file_open_clear(&list);
	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return file;
}

size_t file_read(void* buffer, size_t size, WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(size == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	_fileio_verify(buffer != (void*) 0, 0, "Buffer was NULL.");

	DWORD _num_bytes = (DWORD) size;
	DWORD _bytes_read;

	BOOL success = ReadFile(file->file, (LPVOID) buffer, _num_bytes, (LPDWORD) &_bytes_read, (LPOVERLAPPED) 0);
	_fileio_verify(success != 0, 0, "ReadFile: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (size_t) _bytes_read;
}

size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(size == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	_fileio_verify(buffer != (const void*) 0, 0, "Buffer was NULL.");

	DWORD _num_bytes = (DWORD) size;
	DWORD _bytes_written;

	BOOL success = WriteFile(file->file, (LPCVOID) buffer, _num_bytes, (LPDWORD) &_bytes_written, (LPOVERLAPPED) 0);
	_fileio_verify(success != 0, 0, "Failed to write to file. WinAPI Error Code: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (size_t) _bytes_written;
}

size_t file_read_string(char* str, size_t buffer_size, WrenchFileHandle* file)
{
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

size_t file_write_string(const char* str, WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");
	_fileio_verify(str != (const char*) 0, 0, "String buffer was NULL.");

	size_t str_len = strlen(str);

	char* str_no_r = (char*) malloc(2 * str_len);

	size_t offset = 0;
	for(size_t i = 0; i < str_len; i++) {
		if(str[i] == '\n') {
			str_no_r[offset++] = '\r';
		}
		str_no_r[offset++] = str[i];
	}

	size_t bytes_written = file_write(str_no_r, offset, file);
	free(str_no_r);

	return bytes_written;
}

size_t file_vprintf(WrenchFileHandle* file, const char* format, va_list vlist)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	int num_chars_required = vsnprintf((char*) 0, 0, format, vlist) + 1;

	char* buffer = malloc(num_chars_required);

	vsnprintf(buffer, num_chars_required, format, vlist);

	size_t num_bytes_written = file_write_string(buffer, file);
	free(buffer);

	return num_bytes_written;
}

size_t file_printf(WrenchFileHandle* file, const char* format, ...)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	va_list list;
	va_start(list, format);
	size_t val = file_vprintf(file, format, list);
	va_end(list);

	return val;
}

int file_seek(WrenchFileHandle* file, size_t offset, WrenchFileOrigin origin)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");

	DWORD _move_method = 0;

	switch(origin) {
		case WRENCH_FILE_ORIGIN_START: _move_method = FILE_BEGIN; break;
		case WRENCH_FILE_ORIGIN_CURRENT: _move_method = FILE_CURRENT; break;
		case WRENCH_FILE_ORIGIN_END: _move_method = FILE_END; break;
		default: _fileio_verify_not_reached(EOF, "Invalid origin specified for file seeking."); break;
	}

	LARGE_INTEGER _offset;
	_offset.QuadPart = offset;

	BOOL success = SetFilePointerEx(file->file, _offset, (PLARGE_INTEGER) 0, _move_method);
	_fileio_verify(success != 0, EOF, "SetFilePointerEx: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (success != 0) ? 0 : EOF;
}

size_t file_tell(WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	LARGE_INTEGER _offset;
	_offset.QuadPart = 0;
	LARGE_INTEGER _tell;
	BOOL success = SetFilePointerEx(file->file, _offset, (PLARGE_INTEGER) &_tell, FILE_CURRENT);
	_fileio_verify(success != 0, 0, "SetFilePointerEx: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (size_t) _tell.QuadPart;
}

size_t file_size(WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, 0, "File handle was NULL.");

	if(file_flush(file) != 0) {
		return 0;
	}

	LARGE_INTEGER size;
	BOOL success = GetFileSizeEx(file->file, &size);
	_fileio_verify(success != 0, 0, "GetFileSizeEx: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (size_t) size.QuadPart;
}

int file_flush(WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");

	if(file->may_flush == 0) {
		FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;
		return 0;
	}

	BOOL success = FlushFileBuffers(file->file);
	_fileio_verify(success != 0, EOF, "FlushFileBuffers: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (success) ? 0 : EOF;
}

int file_close(WrenchFileHandle* file)
{
	_fileio_verify(file != (WrenchFileHandle*) 0, EOF, "File handle was NULL.");
	_fileio_verify(file->file != INVALID_HANDLE_VALUE, EOF, "File handle is invalid.");

	BOOL success = CloseHandle(file->file);
	free(file);
	_fileio_verify(success != 0, EOF, "CloseHandle: %lu.", GetLastError());

	FILEIO_ERROR_CONTEXT_STRING = _fileio_message_ok;

	return (success != 0) ? 0 : EOF;
}
