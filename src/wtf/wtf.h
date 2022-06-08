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

#ifndef CORE_WTF_H
#define CORE_WTF_H

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

#include <stdio.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

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
	int32_t collapsed;
} WtfNode;

typedef enum {
	WTF_NUMBER,
	WTF_BOOLEAN,
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
		int32_t boolean;
		struct {
			char* begin;
			char* end;
		} string;
		WtfAttribute* first_array_element;
	};
} WtfAttribute;

WtfNode* wtf_parse(char* buffer, char** error_dest);
void wtf_free(WtfNode* root);

const WtfNode* wtf_first_child(const WtfNode* parent, const char* type_name);
const WtfNode* wtf_next_sibling(const WtfNode* node, const char* type_name);
const WtfNode* wtf_child(const WtfNode* parent, const char* type_name, const char* tag);

const WtfAttribute* wtf_attribute(const WtfNode* node, const char* key);

#ifdef __cplusplus
}
#endif

#endif
