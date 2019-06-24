#ifndef WORKER_THREAD_H
#define WORKER_THREAD_H

#include <mutex>
#include <atomic>
#include <string>
#include <thread>
#include <sstream>
#include <optional>
#include <functional>

#include "window.h"

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

/*
	worker_logger
*/

template <typename T>
worker_logger& worker_logger::operator<<(T data) {
	std::lock_guard<std::mutex> guard(_mutex);
	_stream << data;
	return *this;
}

/*
	worker_thread
*/

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
	ImGui::InputTextMultiline("##nolabel", &log_text, size, ImGuiInputTextFlags_ReadOnly);
	
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
	_result = job(in, _log);
	_ready = true;
}

#endif
