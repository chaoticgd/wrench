#include <sstream>

#include "../command_line.h"
#include "../stream.h"
#include "../formats/fip.h"

int main(int argc, char** argv) {

	std::string src_path;
	std::string dest_path;
	std::string offset_hex;

	po::options_description desc(
		"Converts indexed colour textures in the FIP format to BMP files");
	desc.add_options()
		("src,s",    po::value<std::string>(&src_path)->required(),
			"The input file.")
		("dest,d",   po::value<std::string>(&dest_path)->required(),
			"The output file.")
		("offset,o", po::value<std::string>(&offset_hex)->default_value("0"),
			"The offset in the input file where the header begins.");

	po::positional_options_description pd;
	pd.add("src", 1);
	pd.add("dest", 1);

	if(!parse_command_line_args(argc, argv, desc, pd)) {
		return 0;
	}

	file_stream src(src_path);
	file_stream dest(dest_path, std::ios::out);

	std::stringstream offset_stream;
	offset_stream << std::hex << offset_hex;
	uint32_t offset;
	offset_stream >> offset;
	
	src.push_zero(offset);

	fip_to_bmp(dest, src);

	return 0;
}
