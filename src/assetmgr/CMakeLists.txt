if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	message(FATAL_ERROR "You should build Wrench using the CMakeLists.txt file in the root of the repository.")
endif()

add_executable(asset_codegen
	asset_codegen.cpp
)
target_link_libraries(asset_codegen wrenchwtf wrenchplatform)

add_executable(asset_docgen
	asset_docgen.cpp
)
target_link_libraries(asset_docgen wrenchwtf wrenchplatform)

add_custom_command(
	COMMAND asset_codegen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf ${CMAKE_BINARY_DIR}/_generated_asset_types.inl
	DEPENDS asset_codegen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf
	OUTPUT ${CMAKE_BINARY_DIR}/_generated_asset_types.inl
	COMMENT "Generating asset types"
)

add_custom_command(
	COMMAND asset_docgen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf ${CMAKE_SOURCE_DIR}/docs/asset_reference.md
	DEPENDS asset_docgen ${CMAKE_SOURCE_DIR}/src/assetmgr/asset_schema.wtf
	OUTPUT docs/asset_reference.md
	COMMENT "Generating asset reference"
)

add_custom_target(
	asset_docgen_target ALL
	DEPENDS docs/asset_reference.md
)

add_library(wrench_assetmgr STATIC
	game_info.cpp
	asset_util.cpp
	asset.cpp
	asset_types.cpp
	zipped_asset_bank.cpp
	material_asset.cpp
	${CMAKE_BINARY_DIR}/_generated_asset_types.inl
)
target_link_libraries(wrench_assetmgr wrenchcore wrenchcppparser zip)
target_precompile_headers(wrench_assetmgr PUBLIC asset.h)
