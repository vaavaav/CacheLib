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
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake
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
set(CMAKE_INSTALL_DIR lib/cmake/holpaca CACHE STRING
    "The subdirectory where CMake package config files should be installed")

include(FetchContent)
# --------------------------------------------------------------------- #
set(SPDLOG_INSTALL ON)
FetchContent_Declare(spdlog
  GIT_REPOSITORY https://github.com/gabime/spdlog.git
  GIT_TAG v1.9.2
)
FetchContent_MakeAvailable(spdlog)
# --------------------------------------------------------------------- #
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG        v3.4.0
)
FetchContent_MakeAvailable(tomlplusplus)
# --------------------------------------------------------------------- #
find_package(Threads REQUIRED)
find_package(Protobuf REQUIRED)
message(STATUS "Protobuf version: ${Protobuf_VERSION}")
find_package(gRPC REQUIRED)


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

# Proto + gRPC
get_filename_component(holpaca-proto-file "include/holpaca/protos/holpaca.proto" ABSOLUTE)
get_filename_component(holpaca-proto-path ${holpaca-proto-file} PATH)
set(holpaca-proto-srcs "${holpaca-proto-path}/holpaca.pb.cc")
set(holpaca-proto-hdrs "${holpaca-proto-path}/holpaca.pb.h")
set(holpaca-grpc-srcs  "${holpaca-proto-path}/holpaca.grpc.pb.cc")
set(holpaca-grpc-hdrs  "${holpaca-proto-path}/holpaca.grpc.pb.h")

# Generated sources

add_custom_command(
      OUTPUT ${holpaca-proto-srcs} ${holpaca-proto-hdrs} ${holpaca-grpc-srcs} ${holpaca-grpc-hdrs}
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${holpaca-proto-path}" 
        --cpp_out "${holpaca-proto-path}"
        -I "${holpaca-proto-path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${holpaca-proto-file}"
      DEPENDS "${holpaca-proto-file}")

add_library(holpaca-proto
${holpaca-proto-hdrs}
${holpaca-grpc-hdrs}
${holpaca-proto-srcs}
${holpaca-grpc-srcs}
)

target_link_libraries(holpaca-proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

# ---------------------------------------------------------------------------- #
# > Build Controller

find_package(GSL REQUIRED)

add_executable(controller
  include/holpaca/control-plane
  include/holpaca/Cache.h
  src/control-plane/Main.cpp
  src/control-plane/Controller.cpp
  src/control-plane/ControllerConfig.cpp
  src/control-plane/ProxyManager.cpp
  src/control-plane/StageProxy.cpp
)

target_include_directories(controller
  PUBLIC
  include
)

target_link_libraries(controller
  PUBLIC
  holpaca-proto
  PRIVATE
  Threads::Threads
  tomlplusplus::tomlplusplus
  spdlog::spdlog
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  GSL::gsl
)

# ---------------------------------------------------------------------------- #
# > Build Holpaca 

set(holpaca-header-dirs
  include/holpaca/data-plane
)
foreach(dir ${holpaca-header-dirs})
  file(GLOB_RECURSE headers ${dir}/*.h)
	set(holpaca-headers
		${holpaca-headers}
    ${headers})
endforeach()

set(holpaca-sources
src/data-plane/Stage.cpp
src/data-plane/StageServer.cpp
src/data-plane/StageConfig.cpp
)

add_library(holpaca
  include/holpaca/Cache.h
  ${holpaca-headers}
  ${holpaca-sources}
)

FetchContent_GetProperties(spdlog SOURCE_DIR spdlog_SOURCE_DIR)
FetchContent_GetProperties(tomlplusplus SOURCE_DIR tomplusplus_SOURCE_DIR)

target_include_directories(holpaca
  PUBLIC
  "$<BUILD_INTERFACE:${holpaca_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${spdlog_SOURCE_DIR}/include>"
  "$<BUILD_INTERFACE:${tomlplusplus_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)

target_link_libraries(holpaca
  PUBLIC
  holpaca-proto
  PRIVATE
  spdlog
  tomlplusplus::tomlplusplus
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

install(
  TARGETS holpaca holpaca-proto spdlog tomlplusplus_tomlplusplus
  EXPORT holpaca-config
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
)

install (DIRECTORY "include/holpaca" DESTINATION ${INCLUDE_INSTALL_DIR})

# Install CMake package configuration files for alpaca
include(CMakePackageConfigHelpers)
configure_package_config_file(
	cmake/holpaca-config.cmake.in
	holpaca-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
  PATH_VARS
    INCLUDE_INSTALL_DIR
    CMAKE_INSTALL_DIR
)

install(
  FILES ${CMAKE_CURRENT_BINARY_DIR}/holpaca-config.cmake
  DESTINATION ${CMAKE_INSTALL_DIR}
)
install(
  EXPORT holpaca-config
  NAMESPACE holpaca::
  FILE holpacaTargets.cmake
  DESTINATION ${CMAKE_INSTALL_DIR}
)
