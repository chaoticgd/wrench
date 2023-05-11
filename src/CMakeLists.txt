if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_library(wrenchwtf STATIC
	wtf/wtf.c
	wtf/wtf_writer.cpp
)

add_subdirectory(platform)
add_subdirectory(core)
add_subdirectory(toolwads)
add_subdirectory(engine)
add_subdirectory(assetmgr)
add_subdirectory(gui)
add_subdirectory(launcher)
add_subdirectory(editor)
add_subdirectory(wrenchbuild)

add_executable(vif
	vifcli.cpp
)
target_link_libraries(vif wrenchcore)

add_subdirectory(iso)

add_executable(memmap
	memmap.c
)

add_executable(asset2json
	asset2json.cpp
)
target_include_directories(asset2json PUBLIC ${CMAKE_SOURCE_DIR}/thirdparty/json/single_include)
target_link_libraries(asset2json wrenchcore)

add_executable(unpackbin
	unpackbin.cpp
)
target_link_libraries(unpackbin wrenchcore wrenchengine)

add_executable(memcard
	memcard.cpp
)
target_link_libraries(memcard wrenchcore wrenchgui)
