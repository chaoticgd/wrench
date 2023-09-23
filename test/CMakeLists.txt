if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(runtests
	test_algorithm.cpp
	test_assetmgr.cpp
	test_compression.cpp
	test_cppparser.cpp
	test_elf.cpp
	test_engine.cpp
	test_gltf.cpp
)
target_link_libraries(runtests catch2)
target_link_libraries(runtests wrenchcore wrench_assetmgr wrenchiso wrenchengine wrenchinstancemgr)

add_test(NAME catch2_tests COMMAND runtests)
file(COPY CTestCustom.cmake DESTINATION ${CMAKE_BINARY_DIR})
