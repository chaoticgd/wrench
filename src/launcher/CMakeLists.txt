add_executable(wrenchlauncher
	main.cpp
	global_state.cpp
	oobe.cpp
	mod_list.cpp
	game_list.cpp
	new_mod_screen.cpp
	image_viewer.cpp
)
target_link_libraries(wrenchlauncher wrenchgui wrench_assetmgr)
