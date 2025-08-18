if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(wrenchsaveeditor
	saveeditor.cpp
	memory_card.cpp
)
target_link_libraries(wrenchsaveeditor wrenchcore wrenchcppparser wrenchengine wrenchgui)
