#include <catch2/catch_amalgamated.hpp>
#include <core/gltf.h>
#include <core/filesystem.h>

static bool test_model_file(const char* path, bool print_diff);

TEST_CASE("read/write sample models", "[gltf]") {
	CHECK(test_model_file("test/data/suzanne.glb", true));
	test_model_file("test/data/rigged_figure.glb", false);
}

static bool test_model_file(const char* path, bool print_diff) {
	std::vector<u8> in = read_file(fs::path(std::string(path)));
	auto model = GLTF::read_glb(in);
	std::vector<u8> out = GLTF::write_glb(model);
	return diff_buffers(in, out, 0, DIFF_REST_OF_BUFFER, print_diff, nullptr);
}
