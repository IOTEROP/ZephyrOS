#
# Copyright (c) 2021 IoTerop
# SPDX-License-Identifier: LicenseRef-BSD-5-Clause-Nordic
#

cmake_minimum_required(VERSION 3.5)



#
find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})

project(iowa_client)

#For evaluation version (from github) 
set(IOWA_SDK_BASE ${CMAKE_CURRENT_SOURCE_DIR}/iowa-sdk/iowa)

FILE(GLOB_RECURSE iowa_sources ${IOWA_SDK_BASE}/src/*.c)

# NORDIC SDK APP START
target_sources( app PRIVATE 
    src/main.c
    src/client_platform.c
    ${iowa_sources})

zephyr_include_directories(
    src
    ${IOWA_SDK_BASE}/include
    ${IOWA_SDK_BASE}/include/objects
    ${IOWA_SDK_BASE}/src/coap
    ${IOWA_SDK_BASE}/src/comm
    ${IOWA_SDK_BASE}/src/core
    ${IOWA_SDK_BASE}/src/data
    ${IOWA_SDK_BASE}/src/logger
    ${IOWA_SDK_BASE}/src/lwm2m
    ${IOWA_SDK_BASE}/src/misc
    ${IOWA_SDK_BASE}/src/objects
    ${IOWA_SDK_BASE}/src/oscore
    ${IOWA_SDK_BASE}/src/security
)
            