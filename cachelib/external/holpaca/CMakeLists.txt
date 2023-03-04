cmake_minimum_required(VERSION 3.1)

if (NOT DEFINED PACKAGE_VERSION)
  set(PACKAGE_VERSION "1.0.0")
endif()

project("holpaca" VERSION ${PACKAGE_VERSION} LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
  ${CMAKE_MODULE_PATH})

# When installing in a non-default prefix, this will let
# projects linking against lib<name>.so to find lib<name>.so automatically.
SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(INCLUDE_INSTALL_DIR include CACHE STRING
    "The subdirectory where header files should be installed")
set(LIB_INSTALL_DIR lib CACHE STRING
    "The subdirectory where libraries should be installed")
set(BIN_INSTALL_DIR bin CACHE STRING
    "The subdirectory where binaries should be installed")
set(CMAKE_INSTALL_DIR lib/cmake/${PROJECT_NAME} CACHE STRING
    "The subdirectory where CMake package config files should be installed")

find_package(cppzmq CONFIG REQUIRED)
find_package(Threads REQUIRED)

include(FetchContent)
include(GNUInstallDirs)
# ---------------------------------------------------------------------------- #
# > spdlog --- logging library 
find_package(spdlog CONFIG QUIET)
if (NOT spdlog_FOUND)
  message(STATUS "spdlog NOT FOUND, fetching from sources")
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.8.1
  )
  FetchContent_MakeAvailable(spdlog)
  set_target_properties(spdlog PROPERTIES POSITION_INDEPENDENT_CODE ON)
  install(TARGETS spdlog
    EXPORT ${PROJECT_NAME}-exports
    DESTINATION ${CMAKE_INSTALL_DIR}
  )
endif()
# ---------------------------------------------------------------------------- #

set("${PROJECT_NAME}_HEADER_DIRS"
  include/holpaca
)

foreach(dir "${${PROJECT_NAME}_HEADER_DIRS}")
  file(GLOB_RECURSE headers ${dir}/*.h)
	set("${${PROJECT_NAME}}_HEADERS"
		"${${PROJECT_NAME}_HEADERS}"
    ${headers})
endforeach()

set("${PROJECT_NAME}_SOURCES"
  src/data_plane/stage.cpp
)

add_library(${PROJECT_NAME}
  "${${PROJECT_NAME}_HEADERS}"
	"${${PROJECT_NAME}_SOURCES}"
)

target_include_directories(
  ${PROJECT_NAME}
  PUBLIC
		"$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Threads::Threads
  spdlog::spdlog
  PRIVATE
  cppzmq
)

install(
  TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}-exports
  DESTINATION ${LIB_INSTALL_DIR}
)

install (DIRECTORY "include/${PROJECT_NAME}" DESTINATION ${INCLUDE_INSTALL_DIR})

# Install CMake package configuration files for alpaca
include(CMakePackageConfigHelpers)
configure_package_config_file(
	cmake/${PROJECT_NAME}-config.cmake.in
	${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
  PATH_VARS
    INCLUDE_INSTALL_DIR
    CMAKE_INSTALL_DIR
)
install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
  DESTINATION ${CMAKE_INSTALL_DIR}
)
install(EXPORT ${PROJECT_NAME}-exports
  FILE ${PROJECT_NAME}-targets.cmake
  NAMESPACE ${PROJECT_NAME}::
  DESTINATION ${CMAKE_INSTALL_DIR}
)

add_executable(controller
  "${${PROJECT_NAME}_HEADERS}"
  src/control_plane/stage_proxy.cpp
  src/control_plane/controller.cpp
)

target_link_libraries(controller
  cppzmq
  spdlog::spdlog
)
target_include_directories(
  controller
  PUBLIC
		"$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)