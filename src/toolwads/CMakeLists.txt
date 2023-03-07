# ******************************************************************************
# Build packer
# ******************************************************************************

add_executable(toolwadpacker
	packer.cpp
)
target_link_libraries(toolwadpacker wrenchcore wrenchengine)

# ******************************************************************************
# Gather git tag and commit hash
# ******************************************************************************

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	message("-- Found git")
	file(GLOB GIT_DEPENDS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*")
	add_custom_command(
		COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD > "${CMAKE_BINARY_DIR}/git_tag.tmp" || echo "" > "${CMAKE_BINARY_DIR}/git_tag.tmp"
		DEPENDS ${GIT_DEPENDS}
		OUTPUT "${CMAKE_BINARY_DIR}/git_tag.tmp"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	add_custom_command(
		COMMAND ${GIT_EXECUTABLE} rev-parse HEAD > "${CMAKE_BINARY_DIR}/git_commit.tmp" || echo "" > "${CMAKE_BINARY_DIR}/git_commit.tmp"
		DEPENDS ${GIT_DEPENDS}
		OUTPUT "${CMAKE_BINARY_DIR}/git_commit.tmp"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
else()
	message("-- Cannot find git (not fatal)")
	file(WRITE "${CMAKE_BINARY_DIR}/git_tag.tmp" "")
	file(WRITE "${CMAKE_BINARY_DIR}/git_commit.tmp" "")
endif()

# ******************************************************************************
# Generate wads
# ******************************************************************************

file(GLOB TOOLWADPACKER_DEPENDS CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/data/*")
add_custom_command(
	COMMAND toolwadpacker "${CMAKE_BINARY_DIR}"
	DEPENDS toolwadpacker
		"${CMAKE_BINARY_DIR}/git_tag.tmp"
		"${CMAKE_BINARY_DIR}/git_commit.tmp"
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
