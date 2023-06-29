#include <catch2/catch_amalgamated.hpp>

extern "C" int wrenchmain(int argc, char** argv) {
	return Catch::Session().run(argc, argv);
}
