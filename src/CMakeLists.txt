if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchwtf STATIC
	wtf/wtf.c
	wtf/wtf_writer.cpp
)

add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(cppparser)
add_subdirectory(toolwads)
add_subdirectory(engine)
add_subdirectory(assetmgr)
add_subdirectory(instancemgr)
add_subdirectory(gui)
add_subdirectory(launcher)
add_subdirectory(editor)
add_subdirectory(trainer)
add_subdirectory(wrenchbuild)
add_subdirectory(saveeditor)
add_subdirectory(wrenchvis)

add_executable(vif
	vifcli.cpp
)
target_link_libraries(vif wrenchcore)

add_subdirectory(iso)

add_executable(memmap
	memmap.c
)
target_link_libraries(memmap wrenchplatform)

add_executable(asset2json
	asset2json.cpp
)
target_include_directories(asset2json PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/json/single_include)
target_link_libraries(asset2json wrenchcore)

add_executable(unpackbin
	unpackbin.cpp
)
target_link_libraries(unpackbin wrenchcore wrenchengine)

# Set the windows codepage to UTF-8.
if(WIN32)
	target_sources(asset2json PUBLIC utf8.manifest)
	target_sources(memmap PUBLIC utf8.manifest)
	target_sources(vif PUBLIC utf8.manifest)
	target_sources(wrenchbuild PUBLIC utf8.manifest)
	target_sources(wrencheditor PUBLIC utf8.manifest)
	target_sources(wrenchlauncher PUBLIC utf8.manifest)
	target_sources(wrenchsaveeditor PUBLIC utf8.manifest)
	target_sources(wrenchtrainer PUBLIC utf8.manifest)
	target_sources(wrenchvis PUBLIC utf8.manifest)
endif()
