add_executable(wrencheditor
	main.cpp
	app.cpp
	command_line.cpp
	util.cpp
	tools.cpp
	renderer.cpp
	level.cpp
	editor.cpp
	gui/editor_gui.cpp
	gui/view_3d.cpp
	gui/inspector.cpp
	gui/asset_selector.cpp
)
target_link_libraries(wrencheditor wrenchcore wrenchgui wrench_assetmgr cxxopts)
