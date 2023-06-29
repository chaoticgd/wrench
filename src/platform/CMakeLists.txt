if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

if(WIN32)
	set(WRENCH_FILEIO_SRC fileio_win.c)
	set(WRENCH_PIPEIO_SRC pipeio_win.c)
else()
	set(WRENCH_FILEIO_SRC fileio.c)
	set(WRENCH_PIPEIO_SRC pipeio.c)
endif()

add_library(wrenchplatform STATIC
	${WRENCH_FILEIO_SRC}
	${WRENCH_PIPEIO_SRC}
)
