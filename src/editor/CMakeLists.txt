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
	instanced_collision_recovery.cpp
	gui/editor_gui.cpp
	gui/view_3d.cpp
	gui/inspector.cpp
	gui/transform_inspector.cpp
	gui/pvar_inspector.cpp
	gui/asset_selector.cpp
	gui/collision_fixer.cpp
	gui/model_preview.cpp
	tools/picker_tool.cpp
	tools/selection_tool.cpp
	tools/transform_tool.cpp
)
target_link_libraries(wrencheditor wrenchcore wrenchgui wrench_assetmgr wrenchinstancemgr)
