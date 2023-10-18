#include <catch2/catch_amalgamated.hpp>
#include <core/tristrip.h>

static s32 restart(s32 value) {
	return value | (1 << 31);
}

TEST_CASE("triangle strip conversion", "[tristrip]") {
	std::vector<s32> restart_strip = { 1, 2, 3, restart(4), 5, restart(6), 7, 8, restart(9), 10 };
	std::vector<s32> zero_area_strip = { 1, 2, 3, 3, 4, 5, 5, 6, 7, 8, 8, 9, 10 };
	CHECK(restart_bit_strip_to_zero_area_tris(restart_strip) == zero_area_strip);
	CHECK(zero_area_tris_to_restart_bit_strip(zero_area_strip) == restart_strip);
}
