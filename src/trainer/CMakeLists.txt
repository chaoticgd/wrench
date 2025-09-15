if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(trainer_codegen
	trainer_codegen.cpp
)
target_link_libraries(trainer_codegen wrenchcore wrenchcppparser)

add_custom_command(
	COMMAND trainer_codegen ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_rac.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_gc.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_uya.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_dl.h ${CMAKE_BINARY_DIR}/_generated_trainer_types.inl
	DEPENDS trainer_codegen ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_rac.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_gc.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_uya.h ${CMAKE_SOURCE_DIR}/data/trainer/trainer_types_dl.h
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_trainer_types.inl
	COMMENT "Generating trainer types"
)

add_executable(wrenchtrainer
	main.cpp
	remote.cpp
	trainer.cpp
	${CMAKE_BINARY_DIR}/_generated_trainer_types.inl
)
target_link_libraries(wrenchtrainer wrenchcore wrenchcppparser wrenchengine wrenchgui)
target_link_libraries(wrenchtrainer pine)
