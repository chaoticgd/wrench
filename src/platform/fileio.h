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

#ifndef PLATFORM_FILEIO_H
#define PLATFORM_FILEIO_H

#ifdef __cplusplus
extern "C" {
#endif

// This contains a null-terminated string containing
// information of the last call to a file I/O function.
extern const char* FILEIO_ERROR_CONTEXT_STRING;

#include <stdarg.h>
#include <stdlib.h>

// Opaque structure with platform dependent implementation.
struct _wrench_file_handle;
typedef struct _wrench_file_handle WrenchFileHandle;

enum _wrench_file_mode
{
	// Opens a file with read access, fails if file does not exist.
	WRENCH_FILE_MODE_READ = 1,
	// Opens a file with write access, deletes original file if it exists.
	WRENCH_FILE_MODE_WRITE = 2,
	// Opens a file with write access, keeps original file if it exists and writes to the end of the file.
	WRENCH_FILE_MODE_WRITE_APPEND = 3,
	// Opens a file with read and write access, keeps original file if it exists, fails otherwise.
	WRENCH_FILE_MODE_READ_WRITE_MODIFY = 4,
	// Opens a file with read and write access, deletes original file if it exists.
	WRENCH_FILE_MODE_READ_WRITE_NEW = 5,
	// Opens a file with read and write access, keeps original file if it exists and writes to the end of the file.
	WRENCH_FILE_MODE_READ_WRITE_APPEND = 6
} typedef WrenchFileMode;

enum _wrench_file_origin
{
	// Origin is the beginning of the file.
	WRENCH_FILE_ORIGIN_START = 1,
	// Origin is the position of the current file pointer.
	WRENCH_FILE_ORIGIN_CURRENT = 2,
	// Origin is the end of the file.
	WRENCH_FILE_ORIGIN_END = 3
} typedef WrenchFileOrigin;

/**
 * Opens a file indicated by filename and returns a pointer to an associated
 * file handle.
 *
 * \param filename Name of file to open.
 * \param mode File access mode, see \ref WrenchFileMode.
 * \return Handle for the file on success, NULL else.
 *
 */
WrenchFileHandle* file_open(const char* filename, const WrenchFileMode mode);

/**
 * Reads up to size many bytes from an input file handle.
 * If there are less than size bytes left in the file from the current file pointer,
 * then all bytes until the end of file are read.
 *
 * \param buffer Buffer in which the read bytes are stored.
 * \param size Number of bytes to reads.
 * \param file Handle to a stream with read access.
 * \return Number of bytes read.
 *
 */
size_t file_read(void* buffer, size_t size, WrenchFileHandle* file);

/**
 * Writes size many bytes from the given buffer to an output file handle.
 *
 * \param buffer Buffer to be stored.
 * \param size Number of bytes to write.
 * \param file Handle to a stream with write or append access.
 * \return Number of bytes written.
 *
 */
size_t file_write(const void* buffer, size_t size, WrenchFileHandle* file);

/**
 * Reads a text from the provided file handle.
 * Converts platform dependent line endings into '\n'.
 * This functions assumes that the file may originate from any supported platform
 * and performs line ending conversion accordingly.
 * The returned string will always be null-terminated.
 *
 * \param str Buffer in which the string will be stored.
 * \param buffer_size Size of the provided buffer in bytes.
 * \param file Handle to a stream with read access.
 * \return Size of returned string excluding null-terminator.
 *
 */
size_t file_read_string(char* str, size_t buffer_size, WrenchFileHandle* file);

/**
 * Writes a null-terminated string to a provided file handle.
 * The null-terminator is not written to file.
 * Converts '\n' into platform dependent line endings.
 * This function in intended for use when writing text files.
 *
 * \param str Null-terminated string to be stored.
 * \param file Handle to a stream with write or append access.
 * \return Number of bytes written.
 *
 */
size_t file_write_string(const char* str, WrenchFileHandle* file);

/**
 * Writes a string given by a null-terminated format string and a list
 * of data to a provided file handle.
 * Converts '\n' into platform dependent line endings.
 * This function in intended for use when writing text files.
 *
 * \param file Handle to a stream with write or append access.
 * \param format Pointer to a null-terminated character string specifying how to interpret the data.
 * \param vlist Variable argument list containing the data to print.
 * \return Number of bytes written.
 *
 */
size_t file_vprintf(WrenchFileHandle* file, const char* format, va_list vlist);

/**
 * Writes a string given by a null-terminated format string and a list
 * of data to a provided file handle.
 * Converts '\n' into platform dependent line endings.
 * This function in intended for use when writing text files.
 *
 * \param str Null-terminated string to be stored.
 * \param file Handle to a stream with write or append access.
 * \return Number of bytes written.
 *
 */
size_t file_printf(WrenchFileHandle* file, const char* format, ...);

/**
 * Sets the current file pointer of the given file handle to a position defined
 * by origin with offset added onto it.
 *
 * \param file File handle.
 * \param offset Offset to be applied to origin.
 * \param origin Origin from which to compute the file pointer, see \ref WrenchFileOrigin.
 * \return Returns 0 on success, EOF else.
 *
 */
int file_seek(WrenchFileHandle* file, size_t offset, WrenchFileOrigin origin);

/**
 * Returns the current file pointer of the given file handle.
 *
 * \param file File handle.
 * \return Current file pointer.
 *
 */
size_t file_tell(WrenchFileHandle* file);

/**
 * Gathers the current size of the file associated with the given file handle.
 *
 * \param file File handle.
 * \return Size of file in bytes.
 *
 */
size_t file_size(WrenchFileHandle* file);

/**
 * Forces immediate execution of any write operations to the file associated with the given file handle.
 *
 * \param file File handle.
 * \return Returns 0 on success, EOF else.
 *
 */
int file_flush(WrenchFileHandle* file);

/**
 * Closes a file handle obtained through \ref file_open.
 *
 * \param file File handle.
 * \return Returns 0 on success, EOF else.
 *
 */
int file_close(WrenchFileHandle* file);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_FILEIO_H */
