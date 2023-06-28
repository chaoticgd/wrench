if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(wrencheditor
	main.cpp
	app.cpp
	tools.cpp
	renderer.cpp
	level.cpp
	editor.cpp
	gui/editor_gui.cpp
	gui/view_3d.cpp
	gui/inspector.cpp
	gui/transform_inspector.cpp
	gui/pvar_inspector.cpp
	gui/asset_selector.cpp
)
target_link_libraries(wrencheditor wrenchcore wrenchgui wrench_assetmgr wrenchinstancemgr)
