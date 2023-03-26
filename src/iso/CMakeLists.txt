if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchiso
	iso_unpacker.cpp
	iso_packer.cpp
	iso_tools.cpp
	iso_filesystem.cpp
	table_of_contents.cpp
	wad_identifier.cpp
)
target_link_libraries(wrenchiso wrenchcore wrench_assetmgr)
