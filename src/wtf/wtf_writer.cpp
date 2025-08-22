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

#include "wtf_writer.h"

#include <math.h>
#include <string.h>

static void write_float(char dest[64], float f);

struct WtfWriter
{
	std::string* dest;
	int32_t indent;
	int32_t array_depth;
	char add_blank_line;
	char array_empty;
};

WtfWriter* wtf_begin_file(std::string& dest)
{
	WtfWriter* ctx = (WtfWriter*) malloc(sizeof(WtfWriter));
	ctx->dest = &dest;
	ctx->indent = 0;
	ctx->array_depth = 0;
	ctx->add_blank_line = 0;
	ctx->array_empty = 0;
	return ctx;
}

void wtf_end_file(WtfWriter* ctx)
{
	free(ctx);
}

static void indent(WtfWriter* ctx)
{
	for (int32_t i = 0; i < ctx->indent; i++) {
		*ctx->dest += '\t';
	}
}

void wtf_begin_node(WtfWriter* ctx, const char* type_name, const char* tag)
{
	if (ctx->add_blank_line) {
		indent(ctx);
		*ctx->dest += '\n';
	}
	indent(ctx);
	if (type_name != NULL && strlen(type_name) > 0) {
		*ctx->dest += type_name;
		*ctx->dest += " ";
	}
	*ctx->dest += tag;
	*ctx->dest += " {\n";
	ctx->indent++;
	ctx->add_blank_line = 0;
}

void wtf_end_node(WtfWriter* ctx)
{
	ctx->indent--;
	indent(ctx);
	*ctx->dest += "}\n";
	ctx->add_blank_line = 1;
}

void wtf_begin_attribute(WtfWriter* ctx, const char* key)
{
	indent(ctx);
	*ctx->dest += key;
	*ctx->dest += ": ";
}

void wtf_end_attribute(WtfWriter* ctx)
{
	ctx->add_blank_line = 1;
}

void wtf_write_integer(WtfWriter* ctx, int32_t i)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += std::to_string(i);
	*ctx->dest += "\n";
}

void wtf_write_float(WtfWriter* ctx, float f)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	char string[64] = {0};
	write_float(string, f);
	*ctx->dest += string;
	*ctx->dest += '\n';
}

void wtf_write_boolean(WtfWriter* ctx, bool b)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	if (b) {
		*ctx->dest += "true\n";
	} else {
		*ctx->dest += "false\n";
	}
}

static const char* HEX_DIGITS = "0123456789abcdef";

void wtf_write_string(WtfWriter* ctx, const char* string, const char* string_end)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	
	if (string_end == NULL) {
		string_end = string + strlen(string);
	}
	
	*ctx->dest += '"';
	for (; string < string_end; string++) {
		if (*string == '\t') {
			*ctx->dest += "\\\t";
		} else if (*string == '\n') {
			*ctx->dest += "\\\n";
		} else if (*string == '\"') {
			*ctx->dest += "\\\"";
		} else if (*string == '\\') {
			*ctx->dest += "\\\\";
		} else if (isprint(*string)) {
			*ctx->dest += *string;
		} else {
			char escape_code[5] = {
				'\\', 'x',
				HEX_DIGITS[(*string >> 4) & 0xf],
				HEX_DIGITS[*string & 0xf],
				'\0'
			};
			*ctx->dest += escape_code;
		}
	}
	*ctx->dest += "\"\n";
}

void wtf_begin_array(WtfWriter* ctx)
{
	if (ctx->array_empty) {
		*ctx->dest += '\n';
	}
	ctx->array_empty = 1;
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += '[';
	ctx->indent++;
	ctx->array_depth++;
}

void wtf_end_array(WtfWriter* ctx)
{
	ctx->indent--;
	if (!ctx->array_empty) {
		indent(ctx);
	}
	*ctx->dest += "]\n";
	ctx->array_depth--;
	ctx->array_empty = 0;
}

void wtf_write_integer_attribute(WtfWriter* ctx, const char* key, int32_t i)
{
	wtf_begin_attribute(ctx, key);
	wtf_write_integer(ctx, i);
	wtf_end_attribute(ctx);
}

void wtf_write_boolean_attribute(WtfWriter* ctx, const char* key, bool b)
{
	wtf_begin_attribute(ctx, key);
	wtf_write_boolean(ctx, b);
	wtf_end_attribute(ctx);
}

void wtf_write_float_attribute(WtfWriter* ctx, const char* key, float f)
{
	wtf_begin_attribute(ctx, key);
	wtf_write_float(ctx, f);
	wtf_end_attribute(ctx);
}

void wtf_write_string_attribute(
	WtfWriter* ctx, const char* key, const char* string, const char* string_end)
{
	wtf_begin_attribute(ctx, key);
	wtf_write_string(ctx, string, string_end);
	wtf_end_attribute(ctx);
}

void wtf_write_bytes(WtfWriter* ctx, const uint8_t* bytes, int count)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += "[";
	for (int i = 0; i < count; i++) {
		char string[8] = {0};
		snprintf(string, 8, "%d%s", bytes[i], (i < count - 1) ? " " : "");
		*ctx->dest += string;
	}
	*ctx->dest += "]\n";
}

void wtf_write_floats(WtfWriter* ctx, const float* floats, int count)
{
	if (ctx->array_empty) {
		*ctx->dest += "\n";
		ctx->array_empty = 0;
	}
	if (ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += "[";
	for (int i = 0; i < count; i++) {
		char string[64] = {0};
		write_float(string, floats[i]);
		*ctx->dest += string;
		if (i < count - 1) {
			*ctx->dest += " ";
		}
	}
	*ctx->dest += "]\n";
}

static void write_float(char dest[64], float f)
{
	int classification = fpclassify(f);
	if (classification == FP_NAN) {
		snprintf(dest, 64, "nan");
	} else if (classification == FP_INFINITE) {
		snprintf(dest, 64, "inf");
	} else {
		snprintf(dest, 64, "%.9g", f);
	}
}
