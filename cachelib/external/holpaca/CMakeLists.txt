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
list(APPEND CMAKE_PREFIX_PATH "/usr/lib") # Probably to remove in the future 

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
# ---------------------------------------------------------------------------- #
# > spdlog --- logging library 
find_package(spdlog 1.9.2 CONFIG QUIET)
if (NOT spdlog_FOUND)
  message(STATUS "spdlog NOT FOUND, fetching from sources")
  FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.9.2 
  )
  FetchContent_MakeAvailable(spdlog)
  install(TARGETS spdlog
    EXPORT ${PROJECT_NAME}Targets
    DESTINATION ${LIB_INSTALL_DIR}
  )
endif()
# ---------------------------------------------------------------------------- #
# > grpc 
find_package(gRPC 1.53.0 CONFIG QUIET)
if (NOT gRPC_FOUND)
  message(STATUS "grpc NOT FOUND, fetching from sources")
  FetchContent_Declare(gRPC
    GIT_REPOSITORY https://github.com/grpc/grpc
    GIT_TAG v1.53.0
  )
  set(FETCHCONTENT_QUIET OFF)
  FetchContent_MakeAvailable(gRPC)
  #install(TARGETS gRPC
  #  EXPORT ${PROJECT_NAME}Targets
  #  DESTINATION ${LIB_INSTALL_DIR}
  #)
endif()
# ---------------------------------------------------------------------------- #
# > protobuf
find_package(Protobuf CONFIG REQUIRED)
if (NOT Protobuf_FOUND)
  message(STATUS "Protobuf NOT FOUND, fetching from sources")
  FetchContent_Declare(Protobuf
    GIT_REPOSITORY https://github.com/protocolbuffers/protobuf
    GIT_TAG v3.12.2
  )
  FetchContent_MakeAvailable(Protobuf)
endif()

add_compile_definitions(INCLUDE_GRPC)
set(protobuf_MODULE_COMPATIBLE TRUE)
message(STATUS "Using protobuf ${Protobuf_VERSION}")

set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
if(CMAKE_CROSSCOMPILING)
    find_program(_PROTOBUF_PROTOC protoc)
else()
    set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
endif()

set(_GRPC_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
    set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto file
get_filename_component(holpaca_proto "protos/holpaca.proto" ABSOLUTE)
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

add_library(holpaca_grpc_proto
  ${holpaca_grpc_srcs}
  ${holpaca_grpc_hdrs}
  ${holpaca_proto_srcs}
  ${holpaca_proto_hdrs}
)

target_link_libraries(holpaca_grpc_proto
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)
# ---------------------------------------------------------------------------- #
# > Build Holpaca 

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
  src/data_plane/autonomous_stage.cpp
  src/control_algorithm/naive_control_algorithm.cpp
)

add_library(${PROJECT_NAME}
  "${${PROJECT_NAME}_HEADERS}"
	"${${PROJECT_NAME}_SOURCES}"
)

target_link_libraries(${PROJECT_NAME}
  PUBLIC
  Threads::Threads
  spdlog::spdlog
  gRPC::grpc++
  holpaca_grpc_proto
  PRIVATE
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)

include(GNUInstallDirs)
install (DIRECTORY "include/${PROJECT_NAME}" DESTINATION ${INCLUDE_INSTALL_DIR})

include(CMakePackageConfigHelpers)
configure_package_config_file(
  cmake/${PROJECT_NAME}-config.cmake.in
  ${PROJECT_NAME}-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_DIR}
  PATH_VARS
    INCLUDE_INSTALL_DIR
    CMAKE_INSTALL_DIR
)

target_include_directories(${PROJECT_NAME}
  PUBLIC
  "$<BUILD_INTERFACE:${${PROJECT_NAME}_SOURCE_DIR}/include>"
  "$<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>"
)
install(TARGETS ${PROJECT_NAME}
  EXPORT ${PROJECT_NAME}Targets
  RUNTIME DESTINATION ${BIN_INSTALL_DIR}
  LIBRARY DESTINATION ${LIB_INSTALL_DIR}
  ARCHIVE DESTINATION ${LIB_INSTALL_DIR}
  INCLUDES DESTINATION ${INCLUDE_INSTALL_DIR}
)

# ---------------------------------------------------------------------------- #
# > Build Controller

add_executable(
  controller
  include/holpaca
  src/control_plane/controller.cpp
  src/control_plane/stage_proxy.cpp
)

target_include_directories(
  controller
  PUBLIC
  include
)

target_link_libraries(
  controller
  PUBLIC
  Threads::Threads
  spdlog::spdlog
  gRPC::grpc++
  holpaca_grpc_proto
  PRIVATE
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
)