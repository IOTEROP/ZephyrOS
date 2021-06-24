cmake_minimum_required(VERSION 3.15)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(iowa_client)

# Should be set to the correct directory
set(IOWA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/iowa-sdk/iowa)

# All include files - get only directories
FILE(GLOB_RECURSE iowa_include LIST_DIRECTORIES true ${IOWA_DIR}/*.)

target_include_directories(app PRIVATE 
	${CMAKE_CURRENT_SOURCE_DIR}
	${IOWA_DIR}/include
	${IOWA_DIR}/include/objects
	${iowa_include}
)

# All source files
FILE(GLOB_RECURSE iowa_sources ${IOWA_DIR}/src/*.c)
target_sources(app PRIVATE 
	main.c 
	iowa_platform.c
	 ${iowa_sources} 
)
