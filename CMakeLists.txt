# /****************************************************************************
# Copyright (c) 2013-2014 cocos2d-x.org
# Copyright (c) 2019-present Axmol Engine contributors (see AUTHORS.md).
#
# https://axmol.dev/
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:

# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.

# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
# ****************************************************************************/

cmake_minimum_required(VERSION 3.22...4.1)

set(APP_NAME RoomBreakEscapeNow)

project(${APP_NAME})

if(XCODE)
  set(CMAKE_XCODE_GENERATE_TOP_LEVEL_PROJECT_ONLY TRUE)
endif()

set(_is_axmol_embed FALSE)
set(_AX_USE_PREBUILT FALSE)

set(CMAKE_MODULE_PATH
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/modules
)

# NOTE: The order of the cmake module "include(AXGame...)" statements matters
include(AXGameEngineOptions)
include(AXGameEngineSetup)

# Optional prebuilt OpenSSL 3.0.18 and Opus 1.5.2 libraries, laid out as
# <root>/<platform>/lib*.a for Apple and Android targets.
set(RB_OPENSSL_ROOT "$ENV{RB_OPENSSL_ROOT}" CACHE PATH "Custom OpenSSL 3.0.18 libraries")
set(RB_OPUS_ROOT "$ENV{RB_OPUS_ROOT}" CACHE PATH "Custom Opus 1.5.2 libraries")
if(APPLE OR ANDROID)
  if(ANDROID)
    set(_rb_lib_platform android-arm64)
  elseif(MACOSX)
    set(_rb_lib_platform macos)
  elseif(SIMULATOR OR CMAKE_OSX_SYSROOT MATCHES "simulator")
    set(_rb_lib_platform ios-sim)
  else()
    set(_rb_lib_platform ios-device)
  endif()
  if(RB_OPENSSL_ROOT)
    set_target_properties(OpenSSL::SSL PROPERTIES IMPORTED_LOCATION "${RB_OPENSSL_ROOT}/${_rb_lib_platform}/libssl.a")
    set_target_properties(OpenSSL::Crypto PROPERTIES IMPORTED_LOCATION "${RB_OPENSSL_ROOT}/${_rb_lib_platform}/libcrypto.a")
  endif()
  if(RB_OPUS_ROOT)
    set_target_properties(opus PROPERTIES IMPORTED_LOCATION "${RB_OPUS_ROOT}/${_rb_lib_platform}/libopus.a")
  endif()
endif()

# The common cross-platforms source files and header files
file(GLOB_RECURSE GAME_HEADER CONFIGURE_DEPENDS
  src/*.h src/*.hpp
)

file(GLOB_RECURSE GAME_SOURCE CONFIGURE_DEPENDS
  src/*.cpp src/*.c
)

set(GAME_INC_DIRS
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

set(content_folder
  "${CMAKE_CURRENT_SOURCE_DIR}/assets"
)

if(APPLE)
  ax_mark_multi_resources(common_content_files RES_TO "Resources" FOLDERS ${content_folder})
elseif(WINDOWS)
  ax_mark_multi_resources(common_content_files RES_TO "Content" FOLDERS ${content_folder})
endif()

include(AXGameSourceSetup)

# mark app complie info and libs info
set(APP_SOURCES
  ${GAME_HEADER}
  ${GAME_SOURCE}
)

if(LINUX OR MACOSX OR WINDOWS)
  option(_AX_TESTS "Enable doctest" OFF)
endif()

if(_AX_TESTS)
  set(TEST_TARGET_NAME "${APP_NAME}_test")

  file(GLOB_RECURSE TEST_HEADER CONFIGURE_DEPENDS
    tests/*.h tests/*.hpp
  )
  list(APPEND TEST_HEADER ${GAME_HEADER})

  file(GLOB_RECURSE TEST_SOURCE CONFIGURE_DEPENDS
    tests/*.cpp tests/*.c
  )
  list(APPEND TEST_SOURCE ${GAME_SOURCE})

  set(TEST_TARGET_SOURCES
    ${APP_SOURCES}
    ${TEST_HEADER}
    ${TEST_SOURCE}
  )

 set(TEST_INC_DIRS
   "${CMAKE_CURRENT_SOURCE_DIR}/tests"
   "${CMAKE_CURRENT_SOURCE_DIR}/src"
 )
endif()

include(AXGameTargetSetup)
if(NOT ANDROID)
  set_target_properties(${APP_NAME} PROPERTIES OUTPUT_NAME "RoomBreak Escape Now")
endif()

if(WINDOWS)
  target_include_directories(${APP_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platforms/windows)
endif()

option(RB_VISUAL_PROBE "Build development renderer checks" OFF)
if(RB_VISUAL_PROBE)
  target_sources(${APP_NAME} PRIVATE tests/VisualProbe.cpp tests/MainFlowProbe.cpp)
  target_compile_definitions(${APP_NAME} PRIVATE RB_VISUAL_PROBE=1)
endif()

# mark app resources, resource will be copy auto after mark
ax_setup_app_config(${APP_NAME})

if(_AX_TESTS)
  ax_setup_app_config(${TEST_TARGET_NAME} CONSOLE)
endif()

# Add any libraries you need to link to the project after this point

# Default Platform-specific setup
include(AXGamePlatformSetup)

# Make sure the AXGameFinalSetup is included at the end of this file
include(AXGameFinalSetup)

# CMake's XCODE_EMBED_FRAMEWORKS is ignored by Ninja; copy the same dependency
# explicitly for builds using the macOS Command Line Tools.
if(MACOSX AND NOT XCODE AND TARGET OpenAL)
  add_custom_command(TARGET ${APP_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Contents/Frameworks"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Contents/Frameworks/soft_oal.framework"
    COMMAND /usr/bin/ditto
      "$<TARGET_BUNDLE_DIR:OpenAL>"
      "$<TARGET_BUNDLE_DIR:${APP_NAME}>/Contents/Frameworks/soft_oal.framework"
    VERBATIM)
endif()
