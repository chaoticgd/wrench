#include <catch2/catch_amalgamated.hpp>
#include <core/gltf.h>
#include <core/filesystem.h>

TEST_CASE("read/write suzanne model", "[gltf]") {
	std::vector<u8> in = read_file("test/data/suzanne.glb");
	auto model = GLTF::read_glb(in);
	std::vector<u8> out;
	GLTF::write_glb(out, model);
	write_file("/tmp/out.glb", out);
	REQUIRE(diff_buffers(in, out, 0, DIFF_REST_OF_BUFFER, true, nullptr));
}
