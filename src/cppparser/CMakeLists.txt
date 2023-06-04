if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()
add_library(wrenchcppparser STATIC
	cpp_lexer.cpp
	cpp_type.cpp
	cpp_parser.cpp
)
target_link_libraries(wrenchcppparser wrenchcore)
