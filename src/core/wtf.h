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

// The Wrench Text Format
// ~~~~~~~~~~~~~~~~~~~~~~
//
// This is a self-contained library to read and write WTF-format files e.g. the
// .asset files that are used by Wrench for handling assets. It has been written
// in C in the hope that it may make it easier to use it in other projects,
// although it is a simple format, so that's probably not terribly important.
//
// This parser itself is an in-situ recursive descent parser, so the input
// buffer will be clobbered, and must not be free'd until all string values that
// are needed have been extracted from the tree.
//
// To cleanup the memory allocated during parsing it's sufficient to call free
// on the pointer returned by wtf_parse.

#ifndef CORE_WTF_H
#define CORE_WTF_H

#include <stdio.h>
#include <stdint.h>

// *****************************************************************************

typedef struct WtfNode WtfNode;
typedef struct WtfAttribute WtfAttribute;

typedef struct WtfNode {
	WtfNode* prev_sibling;
	WtfNode* next_sibling;
	WtfNode* first_child;
	WtfAttribute* first_attribute;
	char* type_name;
	char* tag;
} WtfNode;

typedef enum {
	WTF_NUMBER,
	WTF_STRING,
	WTF_ARRAY
} WtfAttributeType;

typedef struct WtfAttribute {
	WtfAttribute* prev;
	WtfAttribute* next;
	char* key;
	WtfAttributeType type;
	union {
		struct {
			int32_t i;
			float f;
		} number;
		char* string;
		WtfAttribute* first_array_element;
	};
} WtfAttribute;

#ifdef __cplusplus
extern "C" {
#endif

WtfNode* wtf_parse(char* buffer, char** error_dest);

// *****************************************************************************

typedef struct {
	FILE* file;
	int32_t indent;
} WtfWriter;

WtfWriter wtf_begin_file(FILE* file);
void wtf_end_file(WtfWriter* ctx);

void wtf_begin_node(WtfWriter* ctx, const char* type_name, const char* tag);
void wtf_end_node(WtfWriter* ctx);

void wtf_attribute(WtfWriter* ctx, const char* key);
void wtf_float(WtfWriter* ctx, float f);
void wtf_string(WtfWriter* ctx, const char* s);

void wtf_begin_array(WtfWriter* ctx, const char* name);
void wtf_end_array(WtfWriter* ctx);

#ifdef __cplusplus
}
#endif

#endif
