if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

if(WIN32)
	set(WRENCH_FILEIO_SRC fileio_win.cpp)
else()
	set(WRENCH_FILEIO_SRC fileio.cpp)
endif()

add_library(wrenchcore STATIC
	buffer.cpp
	instance.cpp
	level.cpp
	util/basic_util.cpp
	util/error_util.cpp
	mesh.cpp
	collada.cpp
	timer.cpp
	png.cpp
	texture.cpp
	vif.cpp
	filesystem.cpp
	${WRENCH_FILEIO_SRC}
	stream.cpp
	memory_profiler.cpp
	shell.cpp
	build_config.cpp
	mesh_graph.cpp
	tristrip.cpp
	material.cpp
	tristrip_packet.cpp
	elf.cpp
	memory_card.cpp
	${CMAKE_SOURCE_DIR}/thirdparty/md5.cpp
)
target_link_libraries(wrenchcore wrenchwtf)
target_link_libraries(wrenchcore Threads::Threads png_static zlibstatic)
target_precompile_headers(wrenchcore PUBLIC util.h)
target_precompile_headers(wrenchcore PUBLIC ../../thirdparty/glm/glm/glm.hpp)
