add_library(wrenchgui
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
target_link_libraries(wrenchgui glfw glm glad imgui nativefiledialog)
