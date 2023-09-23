#include <catch2/catch_amalgamated.hpp>
#include <core/gltf.h>
#include <core/filesystem.h>

static bool test_model_file(const char* path);

TEST_CASE("read/write suzanne model", "[gltf]") {
	CHECK(test_model_file("test/data/suzanne.glb"));
	CHECK(test_model_file("test/data/rigged_figure.glb"));
}

static bool test_model_file(const char* path) {
	std::vector<u8> in = read_file(fs::path(std::string(path)));
	auto model = GLTF::read_glb(in);
	std::vector<u8> out;
	GLTF::write_glb(out, model);
	write_file("/tmp/out.glb", out);
	return diff_buffers(in, out, 0, DIFF_REST_OF_BUFFER, true, nullptr);
}
