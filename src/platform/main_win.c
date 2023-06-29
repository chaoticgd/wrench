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

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>

int wrenchmain(int argc, char** argv);

int wmain(int argc, wchar_t** argv) {
	char** utf8_argv = (char**) malloc((argc + 1) * sizeof(char));
	for(int i = 0; i < argc; i++) {
		int bytes_needed = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) argv[i], -1, NULL, 0, NULL, NULL);
		if(bytes_needed == 0) {
			fprintf(stderr, "error: Failed to convert command line arguments to UTF-8 (2).\n");
			return 1;
		}
		utf8_argv[i] = malloc(bytes_needed);
		int bytes_written = WideCharToMultiByte(CP_UTF8, 0, (LPCWCH) argv[i], -1, (LPSTR) utf8_argv[i], bytes_needed, NULL, NULL);
		if(bytes_written == 0) {
			fprintf(stderr, "error: Failed to convert command line arguments to UTF-8 (2).\n");
			return 1;
		}
	}
	utf8_argv[argc] = NULL;
	return wrenchmain(argc, utf8_argv);
}
