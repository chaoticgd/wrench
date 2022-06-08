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
	WtfNode* next_node;
	WtfAttribute* next_attribute;
	WtfNode* nodes;
	WtfAttribute* attributes;
} WtfReader;

typedef char* ErrorStr;

static ErrorStr count_nodes_and_attributes(WtfReader* ctx);
static void read_nodes_and_attributes(WtfReader* ctx, WtfNode* parent);
static ErrorStr parse_value(WtfReader* ctx, WtfAttribute** attribute_dest);
static ErrorStr parse_float(WtfReader* ctx, float* dest);
static ErrorStr parse_string(WtfReader* ctx, char** dest);
static char* parse_identifier(WtfReader* ctx);
static char peek_char(WtfReader* ctx);
static void advance(WtfReader* ctx);
static void skip_whitespace(WtfReader* ctx);
static void fixup_identifier(char* buffer);
static char* fixup_string(char* buffer);

static char ERROR_STR[128];

WtfNode* wtf_parse(char* buffer, char** error_dest) {
	WtfReader ctx;
	ctx.input = buffer;
	ctx.line = 1;
	ctx.node_count = 1; // Include the root.
	ctx.attribute_count = 0;
	ctx.nodes = NULL;
	ctx.attributes = NULL;
	
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
	allocation_size += ctx.node_count * sizeof(WtfNode);
	allocation_size += ctx.attribute_count * sizeof(WtfAttribute);
	char* allocation = malloc(allocation_size);
	ctx.nodes = (WtfNode*) allocation;
	ctx.attributes = (WtfAttribute*) (allocation + ctx.node_count * sizeof(WtfNode));
	
	// Write out the root.
	WtfNode* root = &ctx.nodes[0];
	memset(root, 0, sizeof(WtfNode));
	
	// Go back to the beginning of the input file.
	ctx.input = buffer;
	ctx.line = 1;
	ctx.next_node = &ctx.nodes[1];
	ctx.next_attribute = &ctx.attributes[0];
	
	// Write out the rest of the nodes and the attributes.
	read_nodes_and_attributes(&ctx, root);
	assert(ctx.next_node == &ctx.nodes[ctx.node_count]
		&& ctx.next_attribute == &ctx.attributes[ctx.attribute_count]);
	
	// Decode escape characters and add null terminators to strings.
	for(int32_t i = 0; i < ctx.node_count; i++) {
		fixup_identifier(ctx.nodes[i].type_name);
		fixup_identifier(ctx.nodes[i].tag);
	}
	for(int32_t i = 0; i < ctx.attribute_count; i++) {
		fixup_identifier(ctx.attributes[i].key);
		if(ctx.attributes[i].type == WTF_STRING) {
			ctx.attributes[i].string.end = fixup_string(ctx.attributes[i].string.begin);
		}
	}
	
	return root;
}

void wtf_free(WtfNode* root) {
	free(root);
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
			
			ErrorStr error = parse_value(ctx, NULL);
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

static void read_nodes_and_attributes(WtfReader* ctx, WtfNode* parent) {
	WtfAttribute* prev_attribute = NULL;
	WtfAttribute** prev_attribute_pointer = &parent->first_attribute;
	
	WtfNode* prev_sibling = NULL;
	WtfNode** prev_node_pointer = &parent->first_child;
	
	char next;
	while(next = peek_char(ctx), (next != '}' && next != '\0')) {
		char* name = parse_identifier(ctx);
		assert(name);
		
		if(peek_char(ctx) == ':') {
			advance(ctx); // ':'
			
			WtfAttribute* attribute = NULL;
			assert(parse_value(ctx, &attribute) == NULL);
			
			attribute->prev = prev_attribute;
			attribute->next = NULL;
			prev_attribute = attribute;
			
			*prev_attribute_pointer = attribute;
			prev_attribute_pointer = &attribute->next;
			
			attribute->key = name;
		} else {
			char* tag = parse_identifier(ctx);
			assert(tag != NULL);
			
			advance(ctx); // '{'
			
			WtfNode* child = ctx->next_node++;
			child->prev_sibling = prev_sibling;
			child->next_sibling = NULL;
			prev_sibling = child;
			
			*prev_node_pointer = child;
			prev_node_pointer = &child->next_sibling;
			
			child->first_child = NULL;
			child->first_attribute = NULL;
			child->type_name = name;
			child->tag = tag;
			
			read_nodes_and_attributes(ctx, child);
			assert(peek_char(ctx) == '}');
			
			advance(ctx); // '}'
		}
	}
}

static ErrorStr parse_value(WtfReader* ctx, WtfAttribute** attribute_dest) {
	WtfAttribute* attribute = NULL;
	
	if(ctx->attributes) {
		attribute = ctx->next_attribute++;
		memset(attribute, 0, sizeof(WtfAttribute));
	}
	
	if(attribute_dest) {
		*attribute_dest = attribute;
	}
	
	switch(peek_char(ctx)) {
		case '\'': {
			char* string;
			ErrorStr error = parse_string(ctx, &string);
			if(error) {
				return error;
			}
			if(ctx->attributes) {
				attribute->type = WTF_STRING;
				attribute->string.begin = string;
			}
			break;
		}
		case '[': {
			WtfAttribute* prev_attribute = NULL;
			WtfAttribute** prev_next_pointer = NULL;
			
			if(ctx->attributes) {
				attribute->type = WTF_ARRAY;
				
				prev_next_pointer = &attribute->first_array_element;
			}
			
			advance(ctx); // '['
			
			while(peek_char(ctx) != ']') {
				WtfAttribute* new_attribute = NULL;
				ErrorStr error = parse_value(ctx, &new_attribute);
				if(error) {
					return error;
				}
				
				if(ctx->attributes) {
					new_attribute->prev = prev_attribute;
					new_attribute->next = NULL;
					prev_attribute = new_attribute;
					*prev_next_pointer = new_attribute;
					prev_next_pointer = &new_attribute->next;
				}
			}
			
			advance(ctx); // ']'
			
			break;
		}
		default: {
			if(strncmp(ctx->input, "false", 5) == 0) {
				if(ctx->attributes) {
					attribute->type = WTF_BOOLEAN;
					attribute->boolean = 0;
				}
				
				ctx->input += 5;
			} else if(strncmp(ctx->input, "true", 4) == 0) {
				if(ctx->attributes) {
					attribute->type = WTF_BOOLEAN;
					attribute->boolean = 1;
				}
				
				ctx->input += 4;
			} else {
				float number;
				ErrorStr error = parse_float(ctx, &number);
				if(error) {
					return error;
				}
				if(ctx->attributes) {
					attribute->type = WTF_NUMBER;
					attribute->number.i = (int32_t) number;
					attribute->number.f = number;
				}
			}
		}
	}
	
	if(!ctx->attributes) {
		ctx->attribute_count++;
	}
	
	return NULL;
}

static ErrorStr parse_float(WtfReader* ctx, float* dest) {
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

static ErrorStr parse_string(WtfReader* ctx, char** dest) {
	advance(ctx); // '\''
	
	char* begin = ctx->input;
	
	char next;
	int escape = 0;
	while(next = peek_char(ctx), ((escape || next != '\'') && next != '\0')) {
		if(!escape) {
			if(next == '\\') {
				escape = 1;
			}
		} else {
			escape = 0;
		}
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
	while(*ctx->input == ' ' || *ctx->input == '\t' || *ctx->input == '\n'
		|| (*ctx->input == '/' && (ctx->input[1] == '/' || ctx->input[1] == '*'))) {
		if(*ctx->input == '/' && ctx->input[1] == '/') {
			while(*ctx->input != '\n' && *ctx->input != '\0') {
				ctx->input++;
			}
		} else if(*ctx->input == '/' && ctx->input[1] == '*') {
			while(*ctx->input != '\0' && !(*ctx->input == '*' && ctx->input[1] == '/')) {
				ctx->input++;
			}
			if(*ctx->input != '\0') {
				ctx->input += 2;
			}
		} else {
			if(*ctx->input == '\n') {
				ctx->line++;
			}
			ctx->input++;
		}
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

static char* fixup_string(char* buffer) {
	if(buffer == NULL) {
		return;
	}
	
	char* dest = buffer;
	char* src = buffer;
	while(*src != '\'' && *src != '\0') {
		if(*src == '\\') {
			src++;
			char c = *(src++);
			if(c == 't') {
				*(dest++) = '\n';
			} else if(c == 'n') {
				*(dest++) = '\t';
			} else if(c == '\'') {
				*(dest++) = '\'';
			} else if(c == '\\') {
				*(dest++) = '\\';
			} else if(c == 'x') {
				unsigned char hi_nibble = *(src++);
				if(hi_nibble == 0) {
					src -= 1;
					break;
				}
				unsigned char lo_nibble = *(src++);
				if(lo_nibble == 0) {
					src -= 2;
					break;
				}
				unsigned char decoded;
				if(hi_nibble >= '0' && hi_nibble <= '9') {
					decoded = (hi_nibble - '0') << 4;
				} else if(hi_nibble >= 'A' && hi_nibble <= 'F') {
					decoded = (hi_nibble - 'A' + 0xa) << 4;
				} else if(hi_nibble >= 'a' && hi_nibble <= 'f') {
					decoded = (hi_nibble - 'a' + 0xa) << 4;
				} else {
					src -= 2;
					break;
				}
				if(lo_nibble >= '0' && lo_nibble <= '9') {
					decoded |= lo_nibble - '0';
				} else if(lo_nibble >= 'A' && lo_nibble <= 'F') {
					decoded |= lo_nibble - 'A' + 0xa;
				} else if(lo_nibble >= 'a' && lo_nibble <= 'f') {
					decoded |= lo_nibble - 'a' + 0xa;
				} else {
					src -= 2;
					break;
				}
				*(dest++) = decoded;
			} else if(c == 0) {
				break;
			}
		} else {
			*(dest++) = *(src++);
		}
	}
	
	*dest = '\0';
	return dest;
}

const WtfNode* wtf_first_child(const WtfNode* parent, const char* type_name) {
	const WtfNode* child = parent->first_child;
	if(type_name != NULL) {
		while(child != NULL && strcmp(child->type_name, type_name) != 0) {
			child = child->next_sibling;
		}
	}
	return child;
}

const WtfNode* wtf_next_sibling(const WtfNode* node, const char* type_name) {
	const WtfNode* sibling = node->next_sibling;
	if(type_name != NULL) {
		while(sibling != NULL && strcmp(sibling->type_name, type_name) != 0) {
			sibling = sibling->next_sibling;
		}
	}
	return sibling;
}

const WtfNode* wtf_child(const WtfNode* parent, const char* type_name, const char* tag) {
	for(const WtfNode* child = parent->first_child; child != NULL; child = child->next_sibling) {
		if((!type_name || strcmp(child->type_name, type_name) == 0)
			&& (!tag || strcmp(child->tag, tag) == 0)) {
			return child;
		}
	}
	return NULL;
}

const WtfAttribute* wtf_attribute(const WtfNode* node, const char* key) {
	for(const WtfAttribute* attribute = node->first_attribute; attribute != NULL; attribute = attribute->next) {
		if(!key || strcmp(attribute->key, key) == 0) {
			return attribute;
		}
	}
	return NULL;
}
