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

#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <atomic>
#include <thread>
#include <optional>
#include <functional>

#include "window.h"
#include "worker_thread.h"

// Must not throw.
template <typename T_out, typename T_in>
using job_function = std::function<std::optional<T_out>(T_in in, worker_logger& log)>;

template <typename T_out, typename T_in>
class worker_thread : public window {
public:
	worker_thread(
		const char* title,
		T_in in,
		job_function<T_out, T_in> job,
		std::function<void(T_out)> on_done);
	~worker_thread();

	const char* title_text() const override;
	ImVec2 initial_size() const override;
	void render(app& a) override;

private:
	void run_thread(T_in in, job_function<T_out, T_in> job);

	const char* _title;
	std::optional<T_out> _result;
	std::atomic<bool> _ready;
	worker_logger _log;
	std::function<void(T_out)> _on_done;
	std::thread _thread;
};

template <typename T_out, typename T_in>
worker_thread<T_out, T_in>::worker_thread(
	const char* title,
	T_in in,
	job_function<T_out, T_in> job,
	std::function<void(T_out)> on_done)
	: _title(title),
	  _ready(false),
	  _on_done(on_done),
	  _thread(&worker_thread::run_thread, this, in, job) {}

template <typename T_out, typename T_in>
worker_thread<T_out, T_in>::~worker_thread() {
	_thread.join();
}

template <typename T_out, typename T_in>
const char* worker_thread<T_out, T_in>::title_text() const {
	return _title;
}

template <typename T_out, typename T_in>
ImVec2 worker_thread<T_out, T_in>::initial_size() const {
	return ImVec2(400, 300);
}

template <typename T_out, typename T_in>
void worker_thread<T_out, T_in>::render(app& a) {
	ImVec2 size = ImGui::GetWindowSize();
	size.x -= 32;
	size.y -= 64;
	std::string log_text = _log.str();
	ImGui::InputTextMultiline("##log", &log_text, size, ImGuiInputTextFlags_ReadOnly);
	
	if(!_ready) {
		return;
	}

	if(_result.has_value()) {
		_on_done(std::move(_result.value()));
		_result = {}; // Only call _on_done once.
	}
	
	if(ImGui::Button("Close")) {
		close(a);
	}
}

template <typename T_out, typename T_in>
void worker_thread<T_out, T_in>::run_thread(T_in in, job_function<T_out, T_in> job) {
	_result = job(std::move(in), _log);
	_ready = true;
}

#endif
