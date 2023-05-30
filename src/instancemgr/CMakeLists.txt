if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(instance_codegen
	instance_codegen.cpp
)
target_link_libraries(instance_codegen wrenchwtf wrenchplatform)

add_executable(instance_docgen
	instance_docgen.cpp
)
target_link_libraries(instance_docgen wrenchwtf wrenchplatform)

add_custom_command(
	COMMAND instance_codegen ${CMAKE_SOURCE_DIR}/src/instancemgr/instance_schema.wtf ${CMAKE_BINARY_DIR}/_generated_instance_types.inl
	DEPENDS instance_codegen ${CMAKE_SOURCE_DIR}/src/instancemgr/instance_schema.wtf
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_instance_types.inl
	COMMENT "Generating instance types"
)

add_custom_command(
	COMMAND instance_docgen ${CMAKE_SOURCE_DIR}/src/instancemgr/instance_schema.wtf ${CMAKE_SOURCE_DIR}/docs/instance_reference.md
	DEPENDS instance_docgen ${CMAKE_SOURCE_DIR}/src/instancemgr/instance_schema.wtf
	OUTPUT docs/instance_reference.md
	COMMENT "Generating instance reference"
)

add_custom_target(
	instance_docgen_target ALL
	DEPENDS docs/instance_reference.md
)

add_library(wrenchinstancemgr STATIC
	cpp_lexer.cpp
	gameplay.cpp
	instance.cpp
	instances.cpp
	level.cpp
	wtf_glue.cpp
	${CMAKE_BINARY_DIR}/_generated_instance_types.inl
)
target_link_libraries(wrenchinstancemgr wrenchcore)
