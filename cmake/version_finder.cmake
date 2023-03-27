find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_SOURCE_DIR}/.git")
	message(STATUS "Found git")
	execute_process(
		COMMAND ${GIT_EXECUTABLE} rev-parse HEAD
		OUTPUT_VARIABLE GIT_COMMIT
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	execute_process(
		COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD
		OUTPUT_VARIABLE GIT_TAG
		OUTPUT_STRIP_TRAILING_WHITESPACE
	)
else()
	message(WARNING "Cannot find git")
	set(GIT_TAG "")
endif()

add_library(versioninfo STATIC
	${CMAKE_SOURCE_DIR}/cmake/version_info.cpp
)
target_compile_definitions(versioninfo PRIVATE -DGIT_COMMIT="${GIT_COMMIT}" -DGIT_TAG="${GIT_TAG}")
