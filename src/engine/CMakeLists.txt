add_library(wrenchengine
	collision.cpp
	compression.cpp
	gameplay.cpp
	moby.cpp
	moby_mesh_exporter.cpp
	moby_mesh_importer.cpp
	moby_animation.cpp
	shrub.cpp
	sky.cpp
	tfrag.cpp
	tie.cpp
	vag.cpp
)
target_link_libraries(wrenchengine wrenchcore)
