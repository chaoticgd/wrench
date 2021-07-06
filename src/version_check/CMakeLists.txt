cmake_minimum_required(VERSION 2.8.11)

# This library will get recompiled on every build, and will provide versioning
# information (git tag and commit) to the rest of the components.
#
# Yes, this is horribly hacky. Stop looking at it.

set(GIT_TAG_RAW "${CMAKE_SOURCE_DIR}/src/version_check/_git_tag_generated_raw.inl")
set(GIT_COMMIT_RAW "${CMAKE_SOURCE_DIR}/src/version_check/_git_commit_generated_raw.inl")
set(GIT_TAG "${CMAKE_SOURCE_DIR}/src/version_check/_git_tag_generated.inl")
set(GIT_COMMIT "${CMAKE_SOURCE_DIR}/src/version_check/_git_commit_generated.inl")
set(QUOTE_BEGIN "${CMAKE_SOURCE_DIR}/src/version_check/quote_begin.txt")
set(QUOTE_END "${CMAKE_SOURCE_DIR}/src/version_check/quote_end.txt")

add_custom_command(
	OUTPUT ${GIT_TAG_RAW}
	COMMAND git tag --points-at HEAD > ${GIT_TAG_RAW}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_command(
	OUTPUT ${GIT_COMMIT_RAW}
	COMMAND git rev-parse HEAD >> ${GIT_COMMIT_RAW}
	WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

if(MSVC)
	add_custom_target(generated_tag ALL
		COMMAND copy "/y /b \"${QUOTE_BEGIN}\" + \"${GIT_TAG_RAW}\" + \"${QUOTE_END}\" \"${GIT_TAG}\""
		DEPENDS ${GIT_TAG_RAW}
	)
	add_custom_target(generated_commit ALL
		COMMAND copy "/y /b \"${QUOTE_BEGIN}\" + \"${GIT_COMMIT_RAW}\" + \"${QUOTE_END}\" \"${GIT_COMMIT}"
		DEPENDS ${GIT_COMMIT_RAW}
	)
else()
	add_custom_target(generated_tag ALL
		COMMAND cat ${QUOTE_BEGIN} ${GIT_TAG_RAW} ${QUOTE_END} > ${GIT_TAG}
		DEPENDS ${GIT_TAG_RAW}
	)
	add_custom_target(generated_commit ALL
		COMMAND cat ${QUOTE_BEGIN} ${GIT_COMMIT_RAW} ${QUOTE_END} > ${GIT_COMMIT}
		DEPENDS ${GIT_COMMIT_RAW}
	)
endif()

add_library(version_check STATIC
	version_check.cpp
)
add_dependencies(version_check generated_tag generated_commit)
