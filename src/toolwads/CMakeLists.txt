if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

# ******************************************************************************
# Build packer
# ******************************************************************************

add_executable(toolwadpacker
	packer.cpp
)
target_link_libraries(toolwadpacker wrenchcore wrenchengine wrenchinstancemgr versioninfo)

# ******************************************************************************
# Generate wads
# ******************************************************************************

file(GLOB_RECURSE TOOLWADPACKER_DEPENDS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/*")
add_custom_command(
	COMMAND toolwadpacker "${CMAKE_BINARY_DIR}"
	DEPENDS toolwadpacker
		"${CMAKE_SOURCE_DIR}/CREDITS"
		"${TOOLWADPACKER_DEPENDS}"
	OUTPUT
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
		"${CMAKE_BINARY_DIR}/memcard.wad"
		"${CMAKE_BINARY_DIR}/trainer.wad"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Generating tool wads"
)

# ******************************************************************************
# Pack headers into executables
# ******************************************************************************

add_executable(header2cpp
	header2cpp.cpp
)
target_link_libraries(header2cpp wrenchplatform)

add_custom_command(
	COMMAND header2cpp "${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp"
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
		"${CMAKE_BINARY_DIR}/memcard.wad"
		"${CMAKE_BINARY_DIR}/trainer.wad"
	DEPENDS header2cpp
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
		"${CMAKE_BINARY_DIR}/memcard.wad"
		"${CMAKE_BINARY_DIR}/trainer.wad"
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp
	COMMENT "Generating _generated_wadinfo.cpp"
)

add_library(wadinfo STATIC
	${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp
	find_wads.cpp
)
