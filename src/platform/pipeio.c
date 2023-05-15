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

#include "pipeio.h"
#include <stdio.h>

static const char* _pipeio_message_ok = "No errors occurred.";
static char _pipeio_message_error_buffer[128];

const char* PIPEIO_ERROR_CONTEXT_STRING = "No errors occurred.";

#define _pipeio_verify(condition, retval, message, ...) \
	{ \
		if(!(condition)) { \
			sprintf(_pipeio_message_error_buffer, message, ##__VA_ARGS__); \
			PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_error_buffer; \
			return retval; \
		} \
	}

#define _pipeio_verify_not_reached(retval, message, ...) \
	{ \
		sprintf(_pipeio_message_error_buffer, message, ##__VA_ARGS__); \
		PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_error_buffer; \
		return retval; \
	}

/*
 * This file provides a standard implementation of the fileio API.
 * This implementation uses only functionalities provided by the POSIX C standard.
 *
 * Platforms that do not implement the POSIX C standard may not function correctly with
 * this implementation and may require their own implementation.
 *
 * Line endings in this implementation are assumed to be '\n'.
 */

struct _wrench_pipe_handle {
	FILE* pipe;
};

WrenchPipeHandle* pipe_open(const char* command, const WrenchPipeMode mode) {
	_pipeio_verify(mode != 0, (WrenchPipeHandle*) 0, "No mode was specified when opening a pipe.");
	_pipeio_verify(command != (const char*) 0, (WrenchPipeHandle*) 0, "Command is NULL.");

	char* mode_string = (char*) 0;
	switch(mode) {
		case WRENCH_PIPE_MODE_READ: mode_string = (char*) "r"; break;
		//case WRENCH_PIPE_MODE_WRITE:
		//    mode_string = (char*) "w";
		//    break;
		default: _pipeio_verify_not_reached((WrenchPipeHandle*) 0, "No valid pipe access mode was specified."); break;
	}

	WrenchPipeHandle* pipe = (WrenchPipeHandle*) malloc(sizeof(WrenchPipeHandle));
	_pipeio_verify(pipe != (WrenchPipeHandle*) 0, (WrenchPipeHandle*) 0, "Failed to allocate WrenchFileHandle.");

	pipe->pipe = popen(command, mode_string);
	_pipeio_verify(pipe->pipe != (FILE*) 0, (WrenchPipeHandle*) 0, "Failed to open pipe for the command %s.", command);

	PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_ok;

	return pipe;
}

char* pipe_gets(char* str, size_t buffer_size, WrenchPipeHandle* pipe) {
	_pipeio_verify(pipe != (WrenchPipeHandle*) 0, (char*) 0, "Pipe handle was NULL.");

	if(buffer_size == 0) {
		PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_ok;
		return 0;
	}

	_pipeio_verify(str != (char*) 0, (char*) 0, "String buffer was NULL.");

	char* retval = fgets(str, buffer_size - 1, pipe->pipe);
	_pipeio_verify(retval != (char*) 0, (char*) 0, "An error occuring in fgets.");

	PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_ok;

	return retval;
}

long pipe_close(WrenchPipeHandle* pipe) {
	_pipeio_verify(pipe != (WrenchPipeHandle*) 0, EOF, "Pipe handle was NULL.");
	_pipeio_verify(pipe->pipe != (FILE*) 0, EOF, "Pipe handle is invalid.");

	int val = pclose(pipe->pipe);
	free(pipe);
	_pipeio_verify(val == 0, EOF, "Failed to close the pipe.");

	PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_ok;

	return val;
}
