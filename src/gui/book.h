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

#ifndef WRENCHGUI_BOOK_H
#define WRENCHGUI_BOOK_H

#include <gui/gui.h>

namespace gui {

struct Page {
	const char* name;
	void (*function)();
};

struct Chapter {
	const char* name;
	const Page* pages;
	s32 count;
};

enum class BookButtons {
	OKAY_CANCEL_APPLY
};

enum class BookResult {
	NONE,
	OKAY,
	CANCEL,
	APPLY
};

BookResult book(const Page** current_page, const char* id, const Chapter* chapters, s32 chapter_count, BookButtons buttons);

}

#endif
