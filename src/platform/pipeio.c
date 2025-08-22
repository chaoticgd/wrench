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

#undef NDEBUG
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

static const char* message_ok = "";
static char error_buffer[128] = "";

const char* PIPEIO_ERROR_CONTEXT_STRING = error_buffer;

struct _wrench_pipe_handle
{
	FILE* pipe;
};

WrenchPipeHandle* pipe_open(const char* command, const WrenchPipeMode mode)
{
	assert(command != NULL);
	assert(mode != 0);

	WrenchPipeHandle* pipe = malloc(sizeof(WrenchPipeHandle));
	if(!pipe) {
		snprintf(error_buffer, sizeof(error_buffer), "malloc: %s", strerror(errno));
		return NULL;
	}
	
	pipe->pipe = popen(command, "r");
	if(!pipe->pipe) {
		snprintf(error_buffer, sizeof(error_buffer), "popen: %s", strerror(errno));
		free(pipe);
		return NULL;
	}

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return pipe;
}

char* pipe_gets(char* str, size_t buffer_size, WrenchPipeHandle* pipe)
{
	assert(pipe);
	
	if(buffer_size == 0) {
		snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
		return 0;
	}
	
	assert(str);
	
	char* ptr = fgets(str, buffer_size, pipe->pipe);
	if(!ptr) {
		snprintf(error_buffer, sizeof(error_buffer), "fgets: %s", strerror(errno));
	}
	
	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return ptr;
}

long pipe_close(WrenchPipeHandle* pipe)
{
	assert(pipe);
	assert(pipe->pipe);

	int val = pclose(pipe->pipe);
	free(pipe);
	if(val != 0) {
		snprintf(error_buffer, sizeof(error_buffer), "pclose: %s", strerror(errno));
	}

	snprintf(error_buffer, sizeof(error_buffer), "%s", message_ok);
	return val;
}
