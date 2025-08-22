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
#include <string.h>

#include <windows.h>
#include <namedpipeapi.h>
#include <processthreadsapi.h>
#include <handleapi.h>

static const char* _pipeio_message_ok = "";
static char _pipeio_message_error_buffer[128];

const char* PIPEIO_ERROR_CONTEXT_STRING = "";

#define _pipeio_verify_not_reached(retval, message, ...) \
	{ \
		sprintf(_pipeio_message_error_buffer, message, ##__VA_ARGS__); \
		PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_error_buffer; \
		return retval; \
	}

#define _pipeio_verify(condition, retval, message, ...) \
	{ \
		if(!(condition)) { \
			_pipeio_verify_not_reached(retval, message, ##__VA_ARGS__); \
		} \
	}

#define _pipeio_verify_with_clear(condition, retval, clear_command, message, ...) \
	{ \
		if(!(condition)) { \
			clear_command; \
			_pipeio_verify_not_reached(retval, message, ##__VA_ARGS__); \
		} \
	}

struct _wrench_pipe_handle
{
	HANDLE pipe;
	HANDLE process;
	HANDLE thread;
};

struct _pipe_open_clear_list
{
	HANDLE read_handle;
	HANDLE write_handle;
	LPWSTR wide_string;
	HANDLE process_handle;
	HANDLE thread_handle;
};

void _pipe_open_clear(struct _pipe_open_clear_list* list) {
	if(list->read_handle != INVALID_HANDLE_VALUE)
		CloseHandle(list->read_handle);

	if(list->write_handle != INVALID_HANDLE_VALUE)
		CloseHandle(list->write_handle);

	if(list->wide_string != NULL)
		free(list->wide_string);

	if(list->process_handle != INVALID_HANDLE_VALUE) {
		TerminateProcess(list->process_handle, 1);
		CloseHandle(list->process_handle);
	}

	if(list->thread_handle != INVALID_HANDLE_VALUE)
		CloseHandle(list->thread_handle);
}

WrenchPipeHandle* pipe_open(const char* command, const WrenchPipeMode mode) {
	_pipeio_verify(mode != 0, (WrenchPipeHandle*) 0, "No mode was specified when opening a pipe.");
	_pipeio_verify(command != (const char*) 0, (WrenchPipeHandle*) 0, "Command is NULL.");

	struct _pipe_open_clear_list list = {.read_handle = INVALID_HANDLE_VALUE,
		.write_handle = INVALID_HANDLE_VALUE,
		.wide_string = NULL,
		.process_handle = INVALID_HANDLE_VALUE,
		.thread_handle = INVALID_HANDLE_VALUE};

	SECURITY_ATTRIBUTES security;
	memset(&security, 0, sizeof(SECURITY_ATTRIBUTES));
	security.nLength = sizeof(SECURITY_ATTRIBUTES);
	security.bInheritHandle = 1;

	BOOL success = CreatePipe(&list.read_handle, &list.write_handle, &security, 0);
	_pipeio_verify_with_clear(success != 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"CreatePipe: %lu.",
		GetLastError());

	success = SetHandleInformation(list.read_handle, HANDLE_FLAG_INHERIT, 0);
	_pipeio_verify_with_clear(success != 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"SetHandleInformation: %lu.",
		GetLastError());

	STARTUPINFOW startup_info;
	memset(&startup_info, 0, sizeof(STARTUPINFOW));

	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.hStdError = list.write_handle;
	startup_info.hStdOutput = list.write_handle;
	startup_info.dwFlags |= STARTF_USESTDHANDLES;

	PROCESS_INFORMATION process_info;
	memset(&process_info, 0, sizeof(PROCESS_INFORMATION));

	LPCCH input_string = (LPCCH) command;
	int wide_string_size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, (LPWSTR) 0, 0);
	_pipeio_verify_with_clear(wide_string_size != 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"MultiByteToWideChar: %lu.",
		GetLastError());

	list.wide_string = (LPWSTR) malloc(wide_string_size * sizeof(WCHAR));
	_pipeio_verify_with_clear(list.wide_string != (LPWSTR) 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"malloc");

	int written_bytes =
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, input_string, -1, list.wide_string, wide_string_size);
	_pipeio_verify_with_clear(written_bytes != 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"MultiByteToWideChar: %lu.",
		GetLastError());

	success = CreateProcessW(NULL, list.wide_string, NULL, NULL, TRUE, 0, NULL, NULL, &startup_info, &process_info);
	_pipeio_verify_with_clear(success != 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"CreateProcessW: %lu.",
		GetLastError());

	list.process_handle = process_info.hProcess;
	list.thread_handle = process_info.hThread;

	WrenchPipeHandle* pipe = (WrenchPipeHandle*) malloc(sizeof(WrenchPipeHandle));
	_pipeio_verify_with_clear(pipe != (WrenchPipeHandle*) 0,
		(WrenchPipeHandle*) 0,
		_pipe_open_clear(&list),
		"Failed to allocate WrenchFileHandle.");

	pipe->pipe = list.read_handle;
	pipe->process = list.process_handle;
	pipe->thread = list.thread_handle;
	list.read_handle = INVALID_HANDLE_VALUE;
	list.process_handle = INVALID_HANDLE_VALUE;
	list.thread_handle = INVALID_HANDLE_VALUE;

	_pipe_open_clear(&list);
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

	DWORD bytes_read;
	BOOL success = ReadFile(pipe->pipe, str, buffer_size - 1, &bytes_read, NULL);
	_pipeio_verify(success != 0, (char*) 0, "ReadFile: %lu.", GetLastError());

	size_t num_bytes = (size_t) bytes_read;

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

	return str;
}

long pipe_close(WrenchPipeHandle* pipe) {
	_pipeio_verify(pipe != (WrenchPipeHandle*) 0, EOF, "Pipe handle was NULL.");
	_pipeio_verify(pipe->pipe != INVALID_HANDLE_VALUE, EOF, "Pipe handle is invalid.");
	_pipeio_verify(pipe->process != INVALID_HANDLE_VALUE, EOF, "Process handle is invalid.");
	_pipeio_verify(pipe->thread != INVALID_HANDLE_VALUE, EOF, "Thread handle is invalid.");

	BOOL success = TerminateProcess(pipe->process, 1);
	DWORD terminate_error = GetLastError();
	// Terminating a process that has already terminated will result in an access denied error which is fine for us.
	_pipeio_verify(success != 0 || terminate_error == ERROR_ACCESS_DENIED,
		EOF,
		"TerminateProcess: %lu.",
		terminate_error);

	DWORD wait_return_code = WaitForSingleObject(pipe->process, 30000);
	_pipeio_verify(wait_return_code != WAIT_FAILED, EOF, "The process did not terminate properly.");
	_pipeio_verify(wait_return_code != WAIT_TIMEOUT, EOF, "The process did not terminate in time.");

	DWORD exit_code;
	success = GetExitCodeProcess(pipe->process, (LPDWORD) &exit_code);
	_pipeio_verify(
		success != 0, EOF, "GetExitCodeProcess: %lu.", GetLastError());

	if(pipe->pipe != INVALID_HANDLE_VALUE) {
		success = CloseHandle(pipe->pipe);
		_pipeio_verify(success != 0, EOF, "CloseHandle: %lu.", GetLastError());
		pipe->pipe = INVALID_HANDLE_VALUE;
	}

	if(pipe->process != INVALID_HANDLE_VALUE) {
		success = CloseHandle(pipe->process);
		_pipeio_verify(success != 0, EOF, "CloseHandle: %lu.", GetLastError());
		pipe->process = INVALID_HANDLE_VALUE;
	}

	if(pipe->thread != INVALID_HANDLE_VALUE) {
		success = CloseHandle(pipe->thread);
		_pipeio_verify(success != 0, EOF, "CloseHandle: %lu.", GetLastError());
		pipe->thread = INVALID_HANDLE_VALUE;
	}

	free(pipe);

	PIPEIO_ERROR_CONTEXT_STRING = _pipeio_message_ok;

	// We don't want to lose the sign bit
	return *((long*) &exit_code);
}
