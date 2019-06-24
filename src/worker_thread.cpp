#include "worker_thread.h"

worker_logger::worker_logger() {}

std::string worker_logger::str() {
	std::string result;
	{
		std::lock_guard<std::mutex> guard(_mutex);
		result = _stream.str();
	}
	return result;
}
