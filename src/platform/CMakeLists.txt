if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

if(WIN32)
	set(WRENCH_FILEIO_SRC fileio_win.c)
else()
	set(WRENCH_FILEIO_SRC fileio.c)
endif()

add_library(wrenchplatform STATIC
	${WRENCH_FILEIO_SRC}
)