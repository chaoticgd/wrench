if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchgui STATIC
	gui.cpp
	config.cpp
	about.cpp
	settings_screen.cpp
	book.cpp
	command_output.cpp
	build_settings.cpp
	commands.cpp
	shaders.cpp
	render_mesh.cpp
)
target_include_directories(wrenchgui PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/imgui)
target_include_directories(wrenchgui PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/nativefiledialog/src/include)
target_link_libraries(wrenchgui wrenchcore wrenchengine wadinfo)
target_link_libraries(wrenchgui glfw glm glad imgui nativefiledialog imguizmo)
