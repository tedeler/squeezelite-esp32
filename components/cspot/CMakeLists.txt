idf_component_register(
	   SRC_DIRS src	protos shim src/platform/esp
	   EXCLUDE_SRCS src/CryptoOpenSSL.cpp
	   INCLUDE_DIRS "include" "protos" "shim"
	   PRIV_REQUIRES codecs json tools pthread mdns mbedtls nvs_flash platform_config services 
)

idf_build_set_property(COMPILE_DEFINITIONS "-DCSPOT_USE_MBEDTLS" APPEND)

target_compile_options(${COMPONENT_LIB} PRIVATE "-std=gnu++17")
target_compile_options(${COMPONENT_LIB} PRIVATE "-fdata-sections")
target_compile_options(${COMPONENT_LIB} PRIVATE "-ffunction-sections")
add_definitions(-Wno-unused-variable -Wno-unused-const-variable -Wchar-subscripts -Wunused-label -Wmaybe-uninitialized -Wmisleading-indentation)

if (!WIN32)
	message(${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_protos.sh)
	execute_process(COMMAND "${CMAKE_CURRENT_SOURCE_DIR}/cmake/generate_protos.sh" WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
endif ()

