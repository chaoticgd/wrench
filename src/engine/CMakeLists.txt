if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchengine STATIC
	collision.cpp
	compression.cpp
	moby.cpp
	moby_mesh_exporter.cpp
	moby_mesh_importer.cpp
	moby_animation.cpp
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
