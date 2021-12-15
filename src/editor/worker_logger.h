/*
	wrench - A set of modding tools for the Ratchet & Clank PS2 games.
	Copyright (C) 2019 chaoticgd

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

#ifndef WORKER_LOGGER_H
#define WORKER_LOGGER_H

#include <string>
#include <mutex>
#include <sstream>

class worker_logger {
public:
	worker_logger();

	template <typename T>
	worker_logger& operator<<(T data);

	std::string str();

private:
	std::mutex _mutex;
	std::stringstream _stream;
};

template <typename T>
worker_logger& worker_logger::operator<<(T data) {
	std::lock_guard<std::mutex> guard(_mutex);
	_stream << data;
	return *this;
}

#endif
