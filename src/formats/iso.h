#ifndef FORMATS_ISO_H
#define FORMATS_ISO_H

#include "../stream.h"

class iso_stream : public file_stream {
public:
	iso_stream(std::string path);

	void populate(app* a) override;
};

#endif
