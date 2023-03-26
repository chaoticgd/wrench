if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

# ******************************************************************************
# Build packer
# ******************************************************************************

add_executable(toolwadpacker
	packer.cpp
)
target_link_libraries(toolwadpacker wrenchcore wrenchengine versioninfo)

# ******************************************************************************
# Generate wads
# ******************************************************************************

file(GLOB TOOLWADPACKER_DEPENDS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/*")
add_custom_command(
	COMMAND toolwadpacker "${CMAKE_BINARY_DIR}"
	DEPENDS toolwadpacker
		"${CMAKE_SOURCE_DIR}/CONTRIBUTORS"
		"${TOOLWADPACKER_DEPENDS}"
	OUTPUT
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Generating tool wads"
)

# ******************************************************************************
# Pack headers into executables (this is mimicking how the game does things)
# ******************************************************************************

add_executable(header2cpp
	header2cpp.cpp
)

add_custom_command(
	COMMAND header2cpp "${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp"
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
	DEPENDS header2cpp
		"${CMAKE_BINARY_DIR}/build.wad"
		"${CMAKE_BINARY_DIR}/gui.wad"
		"${CMAKE_BINARY_DIR}/launcher.wad"
		"${CMAKE_BINARY_DIR}/editor.wad"
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp
	COMMENT "Generating _generated_wadinfo.cpp"
)

add_library(wadinfo
	${CMAKE_BINARY_DIR}/_generated_wadinfo.cpp
	find_wads.cpp
)

# ******************************************************************************
# Zip the underlay
# ******************************************************************************

file(GLOB UNDERLAY_ZIP_DEPENDS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/underlay/*")
add_custom_target(underlay_zip ALL
	COMMAND ${CMAKE_COMMAND} -E tar "cfv" "${CMAKE_BINARY_DIR}/underlay.zip" --format=zip "."
	DEPENDS ${UNDERLAY_ZIP_DEPENDS}
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/data/underlay"
	COMMENT "Generating underlay.zip"
)
