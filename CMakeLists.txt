# Copyright (C) 2026 Oleh Kulykov <olehkulykov@gmail.com>
# All Rights Reserved.
#
# Unauthorized copying of this file, transferring or reproduction of the
# contents of this project, via any medium, is strictly prohibited.
# The contents of this project are proprietary and confidential.

cmake_minimum_required(VERSION 3.31 FATAL_ERROR)

# https://semver.org
set(CLIENT_VERSION_MAJOR 1)
set(CLIENT_VERSION_MINOR 0)
set(CLIENT_VERSION_PATCH 0)

project(CoreTempMgr
  VERSION ${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_PATCH}
  LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CORETEMPMGR_APP coretempmgr)

include(CheckLibraryExists)
include(CheckFunctionExists)
include(CheckIncludeFile)
include(CheckSymbolExists)
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(FindThreads)

if(CMAKE_BUILD_TYPE)
  string(TOLOWER ${CMAKE_BUILD_TYPE} CMAKE_BUILD_TYPE_LOWER)
else()
  set(CMAKE_BUILD_TYPE_LOWER "")
endif()
string(TOLOWER ${CMAKE_GENERATOR} CMAKE_GENERATOR_LOWER)

if (CMAKE_BUILD_TYPE_LOWER STREQUAL "release")
  string(TIMESTAMP R2D9_TIMESTAMP "%Y-%m-%d 00:00:00.00" UTC)
else()
  string(TIMESTAMP R2D9_TIMESTAMP "%Y-%m-%d 00:00:00.00" UTC)
  add_definitions(-DDEBUG=1)
  message("Added definition: DEBUG=1")
endif()

add_definitions(-DR2D9_TIMESTAMP="${R2D9_TIMESTAMP}")
message("Added definition: R2D9_TIMESTAMP=\"${R2D9_TIMESTAMP}\"")

add_definitions(-DVERSION="${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_PATCH}")
message("Added definition: VERSION=\"${CLIENT_VERSION_MAJOR}.${CLIENT_VERSION_MINOR}.${CLIENT_VERSION_PATCH}\"")

if(CMAKE_GENERATOR_LOWER MATCHES "xcode")
  add_definitions(-DXCODE=1)
  message("Added definition: XCODE=1")
endif()

# ------ Definitions ------
# C with -fPIC
check_c_compiler_flag("-fPIC" WITH_FPIC_C)
if (WITH_FPIC_C)
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

# CXX with -fPIC
check_cxx_compiler_flag("-fPIC" WITH_FPIC_CXX)
if (WITH_FPIC_CXX)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC")
endif()

# CXX with -fno-rtti
check_cxx_compiler_flag("-fno-rtti" WITH_FNO_RTTI_CXX)

# ------ Find package ------
# Threads
set(THREADS_PREFER_PTHREAD_FLAG 1)
find_package(Threads REQUIRED)

# lm-sensors
find_library(SENSORS_LIBRARIES
  NAMES sensorsstatic libsensorsstatic sensorsstat libsensorsstat sensors libsensors
)
if(SENSORS_LIBRARIES)
  find_path(SENSORS_INCLUDE_DIRS
    NAMES sensors/sensors.h
  )
  if(SENSORS_INCLUDE_DIRS)
    add_library(Sensors::Sensors INTERFACE IMPORTED)
    target_include_directories(Sensors::Sensors SYSTEM AFTER
      INTERFACE "${SENSORS_INCLUDE_DIRS}"
    )
    target_link_options(Sensors::Sensors
      INTERFACE "${SENSORS_LIBRARIES}"
    )
  endif()
endif()

if(TARGET Sensors::Sensors)
  message("Found lm-sensors include dir(s): ${SENSORS_INCLUDE_DIRS}")
  message("Found lm-sensors lib(s): ${SENSORS_LIBRARIES}")
else()
  if (NOT APPLE)
    message(FATAL_ERROR "lm-sensors not found")
  endif()
endif()

# ---- Sources & headers ----
set(CORETEMPMGR_HEADERS
  src/ctm_config.hpp
  src/ctm_manager.hpp
  src/ctm_sensors.hpp
  src/r2d9.h
  src/r2d9.hpp
  src/r2d9_c_string.h
  src/r2d9_fixed_string_stream.hpp
  src/r2d9_logger.hpp
  src/r2d9_pair.hpp
  src/r2d9_randomizer.hpp
  src/r2d9_rapidjson.hpp
  src/r2d9_semi_fixed_array.hpp
  src/r2d9_time_utils.hpp
  src/r2d9_trio.hpp
)

set(CORETEMPMGR_SOURCES
  src/ctm_config.cpp
  src/ctm_manager.cpp
  src/ctm_sensors.cpp
  src/main.cpp
  src/r2d9_c_string.c
  src/r2d9_logger.cpp
)

# ---- Groups ----
source_group("main"
  FILES
  src/ctm_config.cpp
  src/ctm_config.hpp
  src/ctm_manager.cpp
  src/ctm_manager.hpp
  src/ctm_sensors.cpp
  src/ctm_sensors.hpp
  src/main.cpp
  src/r2d9.h
  src/r2d9.hpp
  src/r2d9_c_string.c
  src/r2d9_c_string.h
  src/r2d9_fixed_string_stream.hpp
  src/r2d9_logger.cpp
  src/r2d9_logger.hpp
  src/r2d9_pair.hpp
  src/r2d9_randomizer.hpp
  src/r2d9_rapidjson.hpp
  src/r2d9_semi_fixed_array.hpp
  src/r2d9_time_utils.hpp
  src/r2d9_trio.hpp
)

add_executable(${CORETEMPMGR_APP} ${CORETEMPMGR_HEADERS} ${CORETEMPMGR_SOURCES})

if (APPLE)
  target_link_libraries(${CORETEMPMGR_APP}
    PRIVATE Threads::Threads
  )
else()
  target_include_directories(${CORETEMPMGR_APP} SYSTEM AFTER
    PRIVATE Sensors::Sensors
  )
  target_link_libraries(${CORETEMPMGR_APP}
    PRIVATE Threads::Threads
    PRIVATE Sensors::Sensors
  )
endif()

set_property(TARGET ${CORETEMPMGR_APP} PROPERTY POSITION_INDEPENDENT_CODE ON)

# ----- Testing -----
enable_testing()

include(CPack)
