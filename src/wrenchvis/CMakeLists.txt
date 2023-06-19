if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(wrenchvis
	wrenchvis.cpp
	visibility.cpp
)
target_link_libraries(wrenchvis wrenchgui wrench_assetmgr wrenchinstancemgr)
target_link_libraries(wrenchvis glfw glm glad)
