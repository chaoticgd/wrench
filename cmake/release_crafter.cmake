# Package up all the files for a release. This is to be run as part of a CI job.
if(ZIP_RELEASE)
	if(GIT_TAG STREQUAL "")
		string(TIMESTAMP RELEASE_DATE "%Y-%m-%d")
		string(SUBSTRING "${GIT_COMMIT}" 0 7 GIT_SHORT_COMMIT)
		set(RELEASE_VERSION "${RELEASE_DATE}-${GIT_SHORT_COMMIT}")
	else()
		set(RELEASE_VERSION ${GIT_TAG})
	endif()
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set(RELEASE_OS "linux")
	elseif(APPLE)
		set(RELEASE_OS "mac")
	elseif(WIN32)
		set(RELEASE_OS "windows")
	else()
		set(RELEASE_OS ${CMAKE_SYSTEM_NAME})
	endif()
	set(RELEASE_NAME "wrench_${RELEASE_VERSION}_${RELEASE_OS}")
	add_custom_target(releasezip ALL
		COMMAND
			${CMAKE_COMMAND} -E make_directory "${CMAKE_BINARY_DIR}/${RELEASE_NAME}" &&
			${CMAKE_COMMAND} -E copy ${RELEASE_FILES} "${CMAKE_BINARY_DIR}/${RELEASE_NAME}" &&
			${CMAKE_COMMAND} -E copy_directory ${RELEASE_DOCS_DIRECTORY} "${CMAKE_BINARY_DIR}/${RELEASE_NAME}/docs" &&
			${CMAKE_COMMAND} -E copy_directory ${RELEASE_LICENSES_DIRECTORY} "${CMAKE_BINARY_DIR}/${RELEASE_NAME}/licenses" &&
			${CMAKE_COMMAND} -E tar "cfv" "${RELEASE_NAME}.zip" --format=zip ${CMAKE_BINARY_DIR}/"${RELEASE_NAME}"
		WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
	)
endif()
