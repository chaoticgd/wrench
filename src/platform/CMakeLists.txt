if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

if(WIN32)
	add_library(wrenchplatform STATIC
		fileio_win.c
		pipeio_win.c
		utf8.manifest
	)
else()
	add_library(wrenchplatform STATIC
		fileio.c
		pipeio.c
	)
endif()
