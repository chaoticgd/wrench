# Build packer

add_executable(toolwadpacker
	packer.cpp
)
target_link_libraries(toolwadpacker wrenchcore wrenchengine)

# Gather git tag and commit hash

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	MESSAGE("-- Found git")
	add_custom_command(
		COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD > "${CMAKE_SOURCE_DIR}/data/git_tag.tmp"
		DEPENDS "${CMAKE_SOURCE_DIR}/*"
		OUTPUT "${CMAKE_SOURCE_DIR}/data/git_tag.tmp"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
	add_custom_command(
		COMMAND ${GIT_EXECUTABLE} rev-parse HEAD > "${CMAKE_SOURCE_DIR}/data/git_commit.tmp"
		DEPENDS "${CMAKE_SOURCE_DIR}/*"
		OUTPUT "${CMAKE_SOURCE_DIR}/data/git_commit.tmp"
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	)
else()
	MESSAGE("-- Cannot find git (not fatal)")
	file(WRITE "${CMAKE_SOURCE_DIR}/data/git_tag.tmp" "")
	file(WRITE "${CMAKE_SOURCE_DIR}/data/git_commit.tmp" "")
endif()

# Generate wads

add_custom_command(
	COMMAND toolwadpacker
	DEPENDS toolwadpacker
		# build.wad
		"${CMAKE_SOURCE_DIR}/data/git_tag.tmp"
		"${CMAKE_SOURCE_DIR}/data/git_commit.tmp"
		# gui.wad
		"${CMAKE_SOURCE_DIR}/CONTRIBUTORS"
		"${CMAKE_SOURCE_DIR}/data/licenses/barlowfont.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/cxxopts.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/glad.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/glfw.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/glm.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/imgui.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/libpng.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/nativefiledialog.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/nlohmanjson.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/rapidxml.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/wrench.txt"
		"${CMAKE_SOURCE_DIR}/data/licenses/zlib.txt"
		"${CMAKE_SOURCE_DIR}/data/gui/Barlow-Regular.ttf"
		"${CMAKE_SOURCE_DIR}/data/gui/Barlow-Italic.ttf"
		# launcher.wad
		"${CMAKE_SOURCE_DIR}/data/launcher/my_mod.png"
		"${CMAKE_SOURCE_DIR}/data/launcher/oobe/wrench_setup.png"
		# editor.wad
		"${CMAKE_SOURCE_DIR}/data/editor/icons/picker_tool.txt"
		"${CMAKE_SOURCE_DIR}/data/editor/icons/selection_tool.txt"
		"${CMAKE_SOURCE_DIR}/data/editor/icons/translate_tool.txt"
		"${CMAKE_SOURCE_DIR}/data/editor/icons/spline_tool.txt"
	OUTPUT
		"${CMAKE_SOURCE_DIR}/data/build.wad"
		"${CMAKE_SOURCE_DIR}/data/gui.wad"
		"${CMAKE_SOURCE_DIR}/data/launcher.wad"
		"${CMAKE_SOURCE_DIR}/data/editor.wad"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	COMMENT "Generating tool wads"
)

# Pack headers into executables (this is mimicking how the game does things)

add_executable(header2cpp
	header2cpp.cpp
)

add_custom_command(
	COMMAND header2cpp _generated_wadinfo.cpp
		"${CMAKE_SOURCE_DIR}/data/build.wad"
		"${CMAKE_SOURCE_DIR}/data/gui.wad"
		"${CMAKE_SOURCE_DIR}/data/launcher.wad"
		"${CMAKE_SOURCE_DIR}/data/editor.wad"
	DEPENDS header2cpp
		"${CMAKE_SOURCE_DIR}/data/build.wad"
		"${CMAKE_SOURCE_DIR}/data/gui.wad"
		"${CMAKE_SOURCE_DIR}/data/launcher.wad"
		"${CMAKE_SOURCE_DIR}/data/editor.wad"
	OUTPUT _generated_wadinfo.cpp
	COMMENT "Generating _generated_wadinfo.cpp"
)

add_library(wadinfo
	_generated_wadinfo.cpp
	find_wads.cpp
)
