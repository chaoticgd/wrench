if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchcore STATIC
	buffer.cpp
	util/basic_util.cpp
	util/error_util.cpp
	mesh.cpp
	collada.cpp
	timer.cpp
	png.cpp
	texture.cpp
	vif.cpp
	filesystem.cpp
	stream.cpp
	memory_profiler.cpp
	shell.cpp
	build_config.cpp
	mesh_graph.cpp
	tristrip.cpp
	material.cpp
	tristrip_packet.cpp
	elf.cpp
	stdout_thread.cpp
	gltf.cpp
	release.cpp
	${CMAKE_SOURCE_DIR}/thirdparty/md5.cpp
)
target_link_libraries(wrenchcore wrenchwtf wrenchplatform)
target_link_libraries(wrenchcore Threads::Threads png_static zlibstatic)
target_include_directories(wrenchcore PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/json/single_include)
target_precompile_headers(wrenchcore PUBLIC util.h)
target_precompile_headers(wrenchcore PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/glm/glm/glm.hpp)
