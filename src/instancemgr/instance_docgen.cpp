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

#include <string>
#include <vector>
#undef NDEBUG
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <filesystem>

#include <platform/fileio.h>
#include <wtf/wtf.h>

static void out(const char* format, ...);

static WrenchFileHandle* out_handle = NULL;

int main(int argc, char** argv)
{
	assert(argc == 3);
	WrenchFileHandle* file = file_open(argv[1], WRENCH_FILE_MODE_READ);
	if (!file) {
		fprintf(stderr, "Failed to open input file.\n");
		return 1;
	}
	std::vector<char> bytes;
	char c;
	while (file_read(&c, 1, file) == 1) {
		bytes.emplace_back(c);
	}
	bytes.push_back(0);
	file_close(file);

	out_handle = file_open(argv[2], WRENCH_FILE_MODE_WRITE);
	assert(out_handle);
	
	char* error = NULL;
	WtfNode* root = wtf_parse((char*) bytes.data(), &error);
	if (error) {
		fprintf(stderr, "Failed to parse instance schema. %s\n", error);
		return 1;
	}
	
	const WtfAttribute* format_version = wtf_attribute(root, "format_version");
	assert(format_version && format_version->type == WTF_NUMBER);
	
	out("# Instance Reference");
	out("");
	out("This file was generated from %s and is for version %d of the instance format.",
		std::filesystem::path(argv[1]).filename().string().c_str(), format_version->number.i);
	out("");
	out("## Instances");
	out("");
	
	for (const WtfNode* type = wtf_first_child(root, "InstanceType"); type != nullptr; type = wtf_next_sibling(type, "InstanceType")) {
		const WtfAttribute* desc = wtf_attribute(type, "desc");
		assert(desc && desc->type == WTF_STRING);
		out("### %s", type->tag);
		out("");
		
		const WtfNode* field = type->first_child;
		if (field) {
			out("");
			out("*Fields*");
			out("");
			out("| Name | Description | Type |");
			out("| - | - | - |");
			for (; field != nullptr; field = field->next_sibling) {
				out("| %s | %s | %s |", field->tag, "", field->type_name);
			}
		}
		out("");
	}
}

static void out(const char* format, ...)
{
	va_list list;
	va_start(list, format);
	file_vprintf(out_handle, format, list);
	file_write("\n", 1, out_handle);
	va_end(list);
}
