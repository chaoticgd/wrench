# Build packer

add_executable(toolwadpacker
	packer.cpp
)
target_link_libraries(toolwadpacker wrenchcore wrenchengine)

# Generate wads

add_custom_command(
	COMMAND toolwadpacker
	DEPENDS toolwadpacker
		${CMAKE_SOURCE_DIR}/data/Barlow-Regular.ttf
		${CMAKE_SOURCE_DIR}/data/launcher/my_mod.png
		${CMAKE_SOURCE_DIR}/data/launcher/oobe/wrench_setup.png
	OUTPUT ${CMAKE_SOURCE_DIR}/data/launcher.wad
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Generating launcher.wad"
)

# Pack headers into executables (this is mimicking how the game does things)

add_executable(header2cpp
	header2cpp.cpp
)

add_custom_command(
	COMMAND header2cpp _generated_wadinfo.cpp ${CMAKE_SOURCE_DIR}/data/launcher.wad
	DEPENDS header2cpp ${CMAKE_SOURCE_DIR}/data/launcher.wad
	OUTPUT _generated_wadinfo.cpp
	COMMENT "Generating _generated_wadinfo.cpp"
)

add_library(wadinfo
	_generated_wadinfo.cpp
)
