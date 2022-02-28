/*
	BSD 2-Clause License

	Copyright (c) 2022 chaoticgd
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	this list of conditions and the following disclaimer in the documentation
	and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "wtf.h"

#include <ctype.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

// *****************************************************************************

typedef struct {
	char* input;
	int32_t line;
	int32_t node_count;
	int32_t attribute_count;
	int32_t next_node;
	int32_t next_attribute;
} WtfReader;

typedef char* ErrorStr;

static ErrorStr count_nodes_and_attributes(WtfReader* ctx);
static void read_nodes_and_attributes(WtfFile* file, int32_t parent, WtfReader* ctx);
static ErrorStr parse_value(int32_t* attribute_dest, WtfFile* file, WtfReader* ctx);
static ErrorStr parse_float(float* dest, WtfReader* ctx);
static ErrorStr parse_string(char** dest, WtfReader* ctx, int destructive);
static char* parse_identifier(WtfReader* ctx);
static char peek_char(WtfReader* ctx);
static void advance(WtfReader* ctx);
static void skip_whitespace(WtfReader* ctx);
static void fixup_identifier(char* buffer);
static void fixup_string(char* buffer);

static char ERROR_STR[128];

WtfFile* wtf_parse(char* buffer, char** error_dest) {
	WtfReader ctx;
	ctx.input = buffer;
	ctx.line = 1;
	ctx.node_count = 1; // Include the root.
	ctx.attribute_count = 0;
	
	// Count the number of nodes and attributes that need to be allocated and
	// do error checking.
	ErrorStr error = count_nodes_and_attributes(&ctx);
	if(error) {
		*error_dest = error;
		return NULL;
	}
	
	if(ctx.input[0] != '\0') {
		snprintf(ERROR_STR, sizeof(ERROR_STR), "Extra '}' at end of file.");
		*error_dest = ERROR_STR;
		return NULL;
	}
	
	// Allocate memory for the nodes and attributes.
	size_t allocation_size = 0;
	allocation_size += sizeof(WtfFile);
	allocation_size += ctx.node_count * sizeof(WtfNode);
	allocation_size += ctx.attribute_count * sizeof(WtfAttribute);
	WtfFile* file = malloc(allocation_size);
	file->nodes = (WtfNode*) ((char*) file + sizeof(WtfFile));
	file->attributes = (WtfAttribute*) ((char*) file + sizeof(WtfFile) + ctx.node_count * sizeof(WtfNode));
	file->node_count = ctx.node_count;
	file->attribute_count = ctx.attribute_count;
	
	// Go back to the beginning of the input file.
	ctx.input = buffer;
	ctx.line = 1;
	ctx.next_node = 1;
	ctx.next_attribute = 0;
	
	// Write out the root.
	file->nodes[0].prev_sibling = -1;
	file->nodes[0].next_sibling = -1;
	file->nodes[0].first_child = 1;
	file->nodes[0].first_attribute = -1;
	file->nodes[0].type_name = NULL;
	file->nodes[0].tag = NULL;
	
	// Write out the rest of the nodes and the attributes.
	read_nodes_and_attributes(file, 0, &ctx);
	assert(ctx.node_count == ctx.next_node && ctx.attribute_count == ctx.next_attribute);
	
	// Decode escape characters and add null terminators to strings.
	for(int32_t i = 0; i < file->node_count; i++) {
		fixup_identifier(file->nodes[i].type_name);
		fixup_identifier(file->nodes[i].tag);
	}
	for(int32_t i = 0; i < file->attribute_count; i++) {
		fixup_identifier(file->attributes[i].key);
		if(file->attributes[i].type == WTF_STRING) {
			fixup_string(file->attributes[i].s);
		}
	}
	
	return file;
}

static ErrorStr count_nodes_and_attributes(WtfReader* ctx) {
	char next;
	while(next = peek_char(ctx), (next != '}' && next != '\0')) {
		char* name = parse_identifier(ctx);
		if(name == NULL) {
			snprintf(ERROR_STR, sizeof(ERROR_STR), "Expected attribute or type name on line %d, got '%16s'.", ctx->line, ctx->input);
			return ERROR_STR;
		}
		
		if(peek_char(ctx) == ':') {
			advance(ctx); // ':'
			
			ErrorStr error = parse_value(NULL, NULL, ctx);
			if(error) {
				return error;
			}
		} else {
			char* tag = parse_identifier(ctx);
			if(tag == NULL) {
				snprintf(ERROR_STR, sizeof(ERROR_STR), "Expected tag on line %d.", ctx->line);
				return ERROR_STR;
			}
			
			if(peek_char(ctx) != '{') {
				snprintf(ERROR_STR, sizeof(ERROR_STR), "Expected '{' on line %d.", ctx->line);
				return ERROR_STR;
			}
			
			advance(ctx); // '{'
			
			ErrorStr error = count_nodes_and_attributes(ctx);
			if(error) {
				return error;
			}
			
			if(peek_char(ctx) != '}') {
				snprintf(ERROR_STR, sizeof(ERROR_STR), "Unexpected end of file.");
				return ERROR_STR;
			}
			
			advance(ctx); // '}'
			
			ctx->node_count++;
		}
	}
	
	return NULL;
}

static void read_nodes_and_attributes(WtfFile* file, int32_t parent, WtfReader* ctx) {
	int32_t prev_attribute = -1;
	int32_t* prev_attribute_pointer = &file->nodes[parent].first_attribute;
	
	int32_t prev_sibling = -1;
	int32_t* prev_node_pointer = &file->nodes[parent].first_child;
	
	char next;
	while(next = peek_char(ctx), (next != '}' && next != '\0')) {
		char* name = parse_identifier(ctx);
		assert(name);
		
		if(peek_char(ctx) == ':') {
			advance(ctx); // ':'
			
			int32_t attribute_index = -1;
			assert(parse_value(&attribute_index, file, ctx) == NULL);
			
			WtfAttribute* attribute = &file->attributes[attribute_index];
			attribute->prev = prev_attribute;
			attribute->next = -1;
			prev_attribute = attribute_index;
			
			*prev_attribute_pointer = attribute_index;
			prev_attribute_pointer = &attribute->next;
			
			attribute->key = name;
		} else {
			char* tag = parse_identifier(ctx);
			assert(tag != NULL);
			
			advance(ctx); // '{'
			
			int32_t child_index = ctx->next_node++;
			WtfNode* child = &file->nodes[child_index];
			child->prev_sibling = prev_sibling;
			child->next_sibling = -1;
			prev_sibling = child_index;
			
			*prev_node_pointer = child_index;
			prev_node_pointer = &child->next_sibling;
			
			child->first_child = -1;
			child->first_attribute = -1;
			child->type_name = name;
			child->tag = tag;
			
			read_nodes_and_attributes(file, child_index, ctx);
			assert(peek_char(ctx) == '}');
			
			advance(ctx); // '}'
		}
	}
}

static ErrorStr parse_value(int32_t* attribute_dest, WtfFile* file, WtfReader* ctx) {
	int32_t attribute_index = ctx->next_attribute++;
	WtfAttribute* attribute = NULL;
	
	if(file) {
		attribute = &file->attributes[attribute_index];
		memset(attribute, 0, sizeof(WtfAttribute));
	}
	
	if(attribute_dest) {
		*attribute_dest = attribute_index;
	}
	
	switch(peek_char(ctx)) {
		case '\'': {
			char* string;
			ErrorStr error = parse_string(&string, ctx, 0);
			if(error) {
				return error;
			}
			if(file) {
				attribute->type = WTF_STRING;
				attribute->s = string;
			}
			break;
		}
		case '[': {
			int32_t prev_attribute = -1;
			int32_t* prev_next_pointer = NULL;
			
			if(file) {
				attribute->type = WTF_ARRAY;
				
				prev_next_pointer = &attribute->first_array_element;
			}
			
			advance(ctx); // '['
			
			while(peek_char(ctx) != ']') {
				int32_t new_attribute = -1;
				ErrorStr error = parse_value(&new_attribute, file, ctx);
				if(error) {
					return error;
				}
				
				if(file) {
					file->attributes[new_attribute].prev = prev_attribute;
					file->attributes[new_attribute].next = -1;
					prev_attribute = new_attribute;
					*prev_next_pointer = new_attribute;
					prev_next_pointer = &file->attributes[new_attribute].next;
				}
			}
			
			advance(ctx); // ']'
			
			break;
		}
		default: {
			float number;
			ErrorStr error = parse_float(&number, ctx);
			if(error) {
				return error;
			}
			if(file) {
				attribute->type = WTF_FLOAT;
				attribute->f = number;
			}
		}
	}
	
	if(!file) {
		ctx->attribute_count++;
	}
	
	return NULL;
}

static ErrorStr parse_float(float* dest, WtfReader* ctx) {
	char* next;
	float value = strtof(ctx->input, &next);
	
	if(next == ctx->input) {
		snprintf(ERROR_STR, sizeof(ERROR_STR), "Failed to parse float on line %d.", ctx->line);
		return ERROR_STR;
	}
	
	ctx->input = next;
	*dest = value;
	return NULL;
}

static ErrorStr parse_string(char** dest, WtfReader* ctx, int destructive) {
	advance(ctx); // '\''
	
	char* begin = ctx->input;
	
	char next;
	while(next = peek_char(ctx), (next != '\'' && next != '\0')) {
		advance(ctx);
	}
	
	if(next == '\0') {
		snprintf(ERROR_STR, sizeof(ERROR_STR), "Unexpected end of file while parsing string.");
		return ERROR_STR;
	}
	
	advance(ctx); // '\''
	
	*dest = begin;
	return NULL;
}

static char* parse_identifier(WtfReader* ctx) {
	skip_whitespace(ctx);
	char* begin = ctx->input;
	while(isalnum(*ctx->input) || *ctx->input == '_') {
		advance(ctx);
	}
	if(begin == ctx->input) {
		return NULL;
	}
	return begin;
}

static char peek_char(WtfReader* ctx) {
	skip_whitespace(ctx);
	return *ctx->input;
}

static void advance(WtfReader* ctx) {
	skip_whitespace(ctx);
	ctx->input++;
}

static void skip_whitespace(WtfReader* ctx) {
	while(*ctx->input == ' ' || *ctx->input == '\t' || *ctx->input == '\n') {
		if(*ctx->input == '\n') {
			ctx->line++;
		}
		ctx->input++;
	}
}

static void fixup_identifier(char* buffer) {
	if(buffer == NULL) {
		return;
	}
	
	while(isalnum(*buffer) || *buffer == '_') {
		buffer++;
	}
	
	*buffer = '\0';
}

static void fixup_string(char* buffer) {
	if(buffer == NULL) {
		return;
	}
	
	char* dest = buffer;
	char* src = buffer;
	while(*src != '\'' && *src != '\0') {
		if(*src == '\\') {
			src++;
			char c = *(src++);
			if(c == 'n') {
				*(dest++) = '\n';
			} else if(c == 't') {
				*(dest++) = '\t';
			}
		} else {
			*(dest++) = *(src++);
		}
	}
	*dest = '\0';
}

// *****************************************************************************

WtfWriter wtf_begin_file(FILE* file) {
	WtfWriter ctx;
	ctx.file = file;
	ctx.indent = 0;
	return ctx;
}

void wtf_end_file(WtfWriter* ctx) {
	ctx->file = NULL;
	ctx->indent = 0;
}

static void indent(WtfWriter* ctx) {
	for(int32_t i = 0; i < ctx->indent; i++) {
		fputc('\t', ctx->file);
	}
}

void wtf_begin_node(WtfWriter* ctx, const char* type_name, const char* tag) {
	indent(ctx);
	fprintf(ctx->file, "%s %s {\n", type_name, tag);
}

void wtf_end_node(WtfWriter* ctx);

void wtf_attribute(WtfWriter* ctx, const char* key);
void wtf_float(WtfWriter* ctx, float f);
void wtf_string(WtfWriter* ctx, const char* s);

void wtf_begin_array(WtfWriter* ctx, const char* name);
void wtf_end_array(WtfWriter* ctx);
