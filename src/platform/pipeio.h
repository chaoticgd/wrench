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

#ifndef PLATFORM_PIPEIO_H
#define PLATFORM_PIPEIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stdlib.h>

// Opaque structure with platform dependent implementation.
struct _wrench_pipe_handle;
typedef struct _wrench_pipe_handle WrenchPipeHandle;

// This contains a null-terminated string containing
// information of the last call to a pipe I/O function.
extern const char* PIPEIO_ERROR_CONTEXT_STRING;

enum _wrench_pipe_mode
{
	// Opens a read-only pipe.
	WRENCH_PIPE_MODE_READ = 1
} typedef WrenchPipeMode;

/**
 * Opens a process by creating a pipe, forking and invoking the shell.
 *
 * \param command Command to execute.
 * \param mode Pipe access mode, see \ref WrenchPipeMode.
 * \return Handle for the pipe on success, NULL else.
 *
 */
WrenchPipeHandle* pipe_open(const char* command, const WrenchPipeMode mode);

/**
 * Reads from a pipe and converts line endings to '\n'.
 * A line ends if a platform dependent line ending is encountered.
 * This functions assumes that the text originates from the same platform and
 * thus has the same line endings.
 * The returned string may contain multiple lines.
 * The returned string will always be null-terminated.
 *
 * \param str Buffer in which the string will be stored.
 * \param buffer_size Size of the provided buffer in bytes.
 * \param file Handle to a pipe with read access.
 * \return Str on success, NULL else.
 *
 */
char* pipe_gets(char* str, size_t buffer_size, WrenchPipeHandle* pipe);

/**
 * Closes a pipe handle obtained through \ref pipe_open.
 *
 * \param pipe File handle.
 * \return Returns exit code of the terminated process on success, EOF else.
 *
 */
long pipe_close(WrenchPipeHandle* pipe);

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_PIPEIO_H */
