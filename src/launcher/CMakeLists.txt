if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

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
