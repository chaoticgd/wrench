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

#ifndef CORE_WTF_WRITER_H
#define CORE_WTF_WRITER_H

#include <core/util.h>

struct WtfWriter;

WtfWriter* wtf_begin_file(std::string& dest);
void wtf_end_file(WtfWriter* ctx);

void wtf_begin_node(WtfWriter* ctx, const char* type_name, const char* tag);
void wtf_end_node(WtfWriter* ctx);

void wtf_begin_attribute(WtfWriter* ctx, const char* key);
void wtf_end_attribute(WtfWriter* ctx);
void wtf_write_integer(WtfWriter* ctx, int32_t i);
void wtf_write_float(WtfWriter* ctx, float f);
void wtf_write_string(WtfWriter* ctx, const char* string);

void wtf_begin_array(WtfWriter* ctx);
void wtf_end_array(WtfWriter* ctx);

#endif
