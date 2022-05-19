/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019-2022 chaoticgd

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

struct WtfWriter {
	std::string* dest;
	int32_t indent;
	int32_t array_depth;
	char add_blank_line;
	char array_empty;
};

WtfWriter* wtf_begin_file(std::string& dest) {
	WtfWriter* ctx = (WtfWriter*) malloc(sizeof(WtfWriter));
	ctx->dest = &dest;
	ctx->indent = 0;
	ctx->array_depth = 0;
	ctx->add_blank_line = 0;
	ctx->array_empty = 0;
	return ctx;
}

void wtf_end_file(WtfWriter* ctx) {
	free(ctx);
}

static void indent(WtfWriter* ctx) {
	for(int32_t i = 0; i < ctx->indent; i++) {
		*ctx->dest += '\t';
	}
}

void wtf_begin_node(WtfWriter* ctx, const char* type_name, const char* tag) {
	if(ctx->add_blank_line) {
		indent(ctx);
		*ctx->dest += '\n';
	}
	indent(ctx);
	*ctx->dest += stringf("%s %s {\n", type_name, tag);
	ctx->indent++;
	ctx->add_blank_line = 0;
}

void wtf_end_node(WtfWriter* ctx) {
	ctx->indent--;
	indent(ctx);
	*ctx->dest += "}\n";
	ctx->add_blank_line = 1;
}

void wtf_begin_attribute(WtfWriter* ctx, const char* key) {
	indent(ctx);
	*ctx->dest += stringf("%s: ", key);
}

void wtf_end_attribute(WtfWriter* ctx) {
	ctx->add_blank_line = 1;
}

void wtf_write_integer(WtfWriter* ctx, int32_t i) {
	if(ctx->array_empty) {
		*ctx->dest += stringf("\n");
		ctx->array_empty = 0;
	}
	if(ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += stringf("%d\n", i);
}

void wtf_write_float(WtfWriter* ctx, float f) {
	if(ctx->array_empty) {
		*ctx->dest += stringf("\n");
		ctx->array_empty = 0;
	}
	if(ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += stringf("%.9g\n", f);
}

void wtf_write_boolean(WtfWriter* ctx, bool b) {
	if(ctx->array_empty) {
		*ctx->dest += stringf("\n");
		ctx->array_empty = 0;
	}
	if(ctx->array_depth > 0) {
		indent(ctx);
	}
	if(b) {
		*ctx->dest += "true\n";
	} else {
		*ctx->dest += "false\n";
	}
}

void wtf_write_string(WtfWriter* ctx, const char* string) {
	if(ctx->array_empty) {
		*ctx->dest += stringf("\n");
		ctx->array_empty = 0;
	}
	if(ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += '\'';
	for(; *string != '\0'; string++) {
		if(*string == '\t') {
			*ctx->dest += "\\\t";
		} else if(*string == '\n') {
			*ctx->dest += "\\\n";
		} else if(*string == '\'') {
			*ctx->dest += "\\\'";
		} else {
			*ctx->dest += *string;
		}
	}
	*ctx->dest += "\'\n";
}

void wtf_begin_array(WtfWriter* ctx) {
	if(ctx->array_empty) {
		*ctx->dest += '\n';
	}
	ctx->array_empty = 1;
	if(ctx->array_depth > 0) {
		indent(ctx);
	}
	*ctx->dest += '[';
	ctx->indent++;
	ctx->array_depth++;
}

void wtf_end_array(WtfWriter* ctx) {
	ctx->indent--;
	if(!ctx->array_empty) {
		indent(ctx);
	}
	*ctx->dest += "]\n";
	ctx->array_depth--;
	ctx->array_empty = 0;
}
