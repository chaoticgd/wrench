#include <sstream>

#include "../command_line.h"
#include "../formats/wad.h"

int main(int argc, char** argv) {
	return run_cli_converter(argc, argv,
		"Decompress WAD segments",
		{
			{ "decompress", decompress_wad }
		});
}
