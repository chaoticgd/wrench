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

#include <Windows.h>
#include <fileapi.h>

struct _wrench_file_handle {
    HANDLE file;
};

WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode) {
    verify(mode != 0, "No mode was specified when opening a file.");
    verify(filename != (const char*) 0, "Filename is NULL.");

	DWORD desired_access = 0;
	DWORD creation_disposition = 0;

	switch (mode)
    {
        case WRENCH_FILE_MODE_READ:
			desired_access = FILE_GENERIC_READ;
            creation_disposition = OPEN_EXISTING;
            break;
        case WRENCH_FILE_MODE_WRITE:
			desired_access = FILE_GENERIC_WRITE;
            creation_disposition = CREATE_ALWAYS;
            break;
        case WRENCH_FILE_MODE_WRITE_APPEND:
			desired_access = FILE_APPEND_DATA;
            creation_disposition = OPEN_ALWAYS;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_MODIFY:
			desired_access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
            creation_disposition = OPEN_EXISTING;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_NEW:
			desired_access = FILE_GENERIC_READ | FILE_GENERIC_WRITE;
            creation_disposition = CREATE_ALWAYS;
            break;
        case WRENCH_FILE_MODE_READ_WRITE_APPEND:
			desired_access = FILE_GENERIC_READ | FILE_APPEND_DATA;
            creation_disposition = OPEN_ALWAYS;
            break;
        default:
            verify_not_reached("No valid file access mode was specified.");
            break;
    }

	DWORD share_mode = FILE_SHARE_READ | FILE_SHARE_WRITE;

	LPCCH input_string = (LPCCH) filename;

	int wide_string_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, (LPWSTR) 0, 0);

	verify(wide_string_size != 0, "Failed to compute wide filename size. WinAPI Error Code: %d.", GetLastError());

	LPWSTR wide_string = (LPWSTR) malloc(wide_string_size * sizeof(WCHAR));

	verify(wide_string != (LPWSTR) 0, "Failed to allocate wide filename.");

	int written_bytes = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, wide_string, wide_string_size);

	verify(written_bytes != 0 && written_bytes <= wide_string_size, "Failed to convert filename. WinAPI Error Code: %d.", GetLastError());

	WrenchFileHandle* file = (WrenchFileHandle*) malloc(sizeof(WrenchFileHandle));

	file->file = CreateFileW(wide_string, desired_access, share_mode, (LPSECURITY_ATTRIBUTES) 0, creation_disposition, FILE_ATTRIBUTE_NORMAL, (HANDLE) 0);

	verify(file->file != INVALID_HANDLE_VALUE, "Failed to open file. WinAPI Error Code: %d.", GetLastError());

	free(wide_string);

	return file;
}

size_t file_read(void* buffer, size_t size, WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	if (size == 0) {
		return 0;
	}

	verify(buffer != (void*) 0, "Buffer was NULL.");

	DWORD _num_bytes = (DWORD) size;
	DWORD _bytes_read;

	WINBOOL success = ReadFile(file->file, (LPVOID) buffer, _num_bytes, (LPDWORD) &_bytes_read, (LPOVERLAPPED) 0);

	verify(success != 0, "Failed to read from file. WinAPI Error Code: %d.", GetLastError());

	return (size_t) _bytes_read;
}

size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	if (size == 0) {
		return 0;
	}

	verify(buffer != (const void*) 0, "Buffer was NULL.");

	DWORD _num_bytes = (DWORD) size;
	DWORD _bytes_written;

	WINBOOL success = WriteFile(file->file, (LPCVOID) buffer, _num_bytes, (LPDWORD) &_bytes_written, (LPOVERLAPPED) 0);

	verify(success != 0, "Failed to write to file. WinAPI Error Code: %d.", GetLastError());

	return (size_t) _bytes_written;
}

size_t file_read_string(char* str, size_t buffer_size, WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	if (buffer_size == 0) {
		return 0;
	}

	verify(str != (char*) 0, "String buffer was NULL.");

	size_t num_bytes = file_read(str, buffer_size - 1, file);

	size_t offset = 0;
	for (size_t i = 0; i < num_bytes; i++) {
		if (str[i] == '\r') {
			str[i] = '\0';
			continue;
		}

		str[offset++] = str[i];
	}

	for (size_t i = offset; i < buffer_size; i++) {
		str[i] = '\0';
	}

	return offset;
}

size_t file_write_string(const char* str, WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");
	verify(str != (char*) 0, "String buffer was NULL.");

	size_t str_len = strlen(str);

	char* str_no_r = (char*) malloc(2 * str_len);

	size_t offset = 0;
	for (size_t i = 0; i < str_len; i++) {
		if (str[i] == '\n') {
			str_no_r[offset++] = '\r';
		}
		str_no_r[offset++] = str[i];
	}

	size_t bytes_written = file_write(str_no_r, offset, file);

	free(str_no_r);

	return bytes_written;
}

int file_seek(WrenchFileHandle* file, s64 offset, WrenchFileOrigin origin) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	DWORD _move_method = 0;

	switch (origin) {
		case WRENCH_FILE_ORIGIN_START:
			_move_method = FILE_BEGIN;
			break;
		case WRENCH_FILE_ORIGIN_CURRENT:
			_move_method = FILE_CURRENT;
			break;
		case WRENCH_FILE_ORIGIN_END:
			_move_method = FILE_END;
			break;
		default:
			verify_not_reached("Invalid origin specified for file seeking.");
			break;
	}

	LARGE_INTEGER _offset;
	_offset.QuadPart = offset;

	WINBOOL success = SetFilePointerEx(file->file, _offset, (PLARGE_INTEGER) 0, _move_method);

	verify(success != 0, "Failed to seek file. WinAPI Error Code: %d.", GetLastError());

	return (success != 0) ? 0 : EOF;
}

s64 file_tell(WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	DWORD _move_method = FILE_CURRENT;

	LARGE_INTEGER _offset;
	_offset.QuadPart = 0;

	LARGE_INTEGER _tell;

	WINBOOL success = SetFilePointerEx(file->file, _offset, (PLARGE_INTEGER) &_tell, _move_method);

	verify(success != 0, "Failed to seek file. WinAPI Error Code: %d.", GetLastError());

	return (s64) _tell.QuadPart;
}

s64 file_size(WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	file_flush(file);

	LARGE_INTEGER size;
	WINBOOL success = GetFileSizeEx(file->file, &size);

	verify(success != 0, "Failed to retrieve file size. WinAPI Error Code: %d.", GetLastError());

	return (s64) size.QuadPart;
}

int file_flush(WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

    WINBOOL success = FlushFileBuffers(file->file);

    return (success) ? 0 : EOF;
}

int file_close(WrenchFileHandle* file) {
	verify(file != (WrenchFileHandle*) 0, "File handle was NULL.");

	WINBOOL success = 0;
	if (file->file != INVALID_HANDLE_VALUE) {
		success = CloseHandle(file->file);
		verify(success != 0, "Failed to close the file. WinAPI Error Code: %d.", GetLastError());
	}

	free(file);

    return (success != 0) ? 0 : EOF;
}
