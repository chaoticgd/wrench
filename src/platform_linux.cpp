#include "platform.h"

#include <boost/process.hpp>

void open_in_browser(std::string url) {
	boost::process::spawn("/usr/bin/xdg-open", url);
}
