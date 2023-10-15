if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchengine STATIC
	collision.cpp
	compression.cpp
	moby_animation.cpp
	moby_high.cpp
	moby_low.cpp
	moby_packet.cpp
	moby_skinning.cpp
	moby_vertex.cpp
	occlusion.cpp
	shrub.cpp
	sky.cpp
	tfrag_low.cpp
	tfrag_high.cpp
	tfrag_debug.cpp
	tie.cpp
	vag.cpp
)
target_link_libraries(wrenchengine wrenchcore)
