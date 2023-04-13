cmake_minimum_required(VERSION 3.13)

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

include(GNUInstallDirs)

find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
message(STATUS "Protobuf version: ${Protobuf_VERSION}")
find_package(gRPC REQUIRED)
include(FetchContent)

# ---------------------------------------------------------------------------- #
# > spdlog --- logging library
message(STATUS "spdlog NOT FOUND, fetching from sources")
set(SPDLOG_INSTALL ON)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2
)
FetchContent_MakeAvailable(spdlog)
# ---------------------------------------------------------------------------- #

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
set(_GRPC_GRPCPP gRPC::grpc++)

message(STATUS "Using gRPC ${gRPC_VERSION}")

if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC protobuf::protoc)
endif()

set(_GRPC_GRPCPP grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(holpaca_proto "include/holpaca/protos/holpaca.proto" ABSOLUTE)
get_filename_component(holpaca_proto_path "${holpaca_proto}" PATH)

# Generated sources
set(holpaca_proto_srcs "${holpaca_proto_path}/holpaca.pb.cc")
set(holpaca_proto_hdrs "${holpaca_proto_path}/holpaca.pb.h")
set(holpaca_grpc_srcs  "${holpaca_proto_path}/holpaca.grpc.pb.cc")
set(holpaca_grpc_hdrs  "${holpaca_proto_path}/holpaca.grpc.pb.h")
add_custom_command(
      OUTPUT "${holpaca_proto_srcs}" "${holpaca_proto_hdrs}" "${holpaca_grpc_srcs}" "${holpaca_grpc_hdrs}"
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${holpaca_proto_path}"
        --cpp_out "${holpaca_proto_path}"
        -I "${holpaca_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${holpaca_proto}"
      DEPENDS "${holpaca_proto}")

add_library(holpaca_proto
  ${holpaca_proto_hdrs}
  ${holpaca_grpc_hdrs}
  ${holpaca_proto_srcs}
  ${holpaca_grpc_srcs}
)

target_link_libraries( holpaca_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# ---------------------------------------------------------------------------- #
# > Build Controller

add_executable(controller
  include/holpaca/control_plane
  include/holpaca/common/status.h
  include/holpaca/common/cache.h
  include/holpaca/config.h
  #src/control_plane/controller.cpp
  src/control_plane/cache_proxy.cpp
  src/control_plane/main.cpp
)

target_include_directories(controller
  PUBLIC
  include
)

target_link_libraries(controller
  PUBLIC
  holpaca_proto
  Threads::Threads
  spdlog::spdlog
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# ---------------------------------------------------------------------------- #
# > Build Holpaca 

#set("${PROJECT_NAME}_HEADER_DIRS"
#  include/holpaca/data_plane
#  include/holpaca/control_algorithm
#)
#
#foreach(dir "${${PROJECT_NAME}_HEADER_DIRS}")
#  file(GLOB_RECURSE headers ${dir}/*.h)
#	set("${${PROJECT_NAME}}_HEADERS"
#		"${${PROJECT_NAME}_HEADERS}"
#    ${headers})
#endforeach()
#
#set("${PROJECT_NAME}_SOURCES"
#  src/data_plane/autonomous_stage.cpp
#  src/control_algorithm/naive_control_algorithm.cpp
#  #src/data_plane/stage_server.cpp
#)
#
#add_library(${PROJECT_NAME}
#  "${${PROJECT_NAME}_HEADERS}"
#	"${${PROJECT_NAME}_SOURCES}"
#)
#
#
#target_include_directories(${PROJECT_NAME}
#  PUBLIC
#  "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>"
#  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
#)
#
#target_link_libraries(${PROJECT_NAME} 
#  PUBLIC
#  holpaca_proto
#  spdlog
#  ${_REFLECTION}
#  ${_GRPC_GRPCPP}
#  ${_PROTOBUF_LIBPROTOBUF}
#)
#
#install(
#  TARGETS ${PROJECT_NAME} 
#  spdlog 
#  holpaca_proto
#  EXPORT ${PROJECT_NAME}-exports
#  DESTINATION ${LIB_INSTALL_DIR}
#)
#
#install (DIRECTORY "include/${PROJECT_NAME}" DESTINATION ${INCLUDE_INSTALL_DIR})
#
## Install CMake package configuration files for alpaca
#include(CMakePackageConfigHelpers)
#configure_package_config_file(
#	cmake/${PROJECT_NAME}-config.cmake.in
#	${PROJECT_NAME}-config.cmake
#  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
#  PATH_VARS
#    INCLUDE_INSTALL_DIR
#    CMAKE_INSTALL_DIR
#)
#
#install(
#  FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}-config.cmake
#  DESTINATION ${CMAKE_INSTALL_DIR}
#)
#install(
#  EXPORT ${PROJECT_NAME}-exports
#  FILE ${PROJECT_NAME}Targets.cmake
#  NAMESPACE ${PROJECT_NAME}::
#  DESTINATION ${CMAKE_INSTALL_DIR}
#)
