set(_PROTOBUF_LIBPROTOBUF protobuf::libprotobuf)
set(_REFLECTION gRPC::grpc++_reflection)
set(_PROTOBUF_PROTOC $<TARGET_FILE:protobuf::protoc>)
set(_GRPC_GRPCPP gRPC::grpc++)

if(CMAKE_CROSSCOMPILING)
  find_program(_PROTOBUF_PROTOC protoc)
  find_program(_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin)
else()
  set(_PROTOBUF_PROTOC protobuf::protoc)
  set(_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
endif()

# Proto + gRPC
get_filename_component(holpaca_proto_file "${CMAKE_CURRENT_LIST_DIR}/protos/Holpaca.proto" ABSOLUTE)
get_filename_component(holpaca_proto_path ${holpaca_proto_file} PATH)

# Generated sources
set(holpaca_proto_srcs "${holpaca_proto_path}/Holpaca.pb.cc")
set(holpaca_proto_hdrs "${holpaca_proto_path}/Holpaca.pb.h")
set(holpaca_grpc_srcs "${holpaca_proto_path}/Holpaca.grpc.pb.cc")
set(holpaca_grpc_hdrs "${holpaca_proto_path}/Holpaca.grpc.pb.h")

add_custom_command(
      OUTPUT ${holpaca_proto_srcs} ${holpaca_proto_hdrs} ${holpaca_grpc_srcs} ${holpaca_grpc_hdrs}
      COMMAND ${_PROTOBUF_PROTOC}
      ARGS --grpc_out "${holpaca_proto_path}" 
        --cpp_out "${holpaca_proto_path}"
        -I "${holpaca_proto_path}"
        --plugin=protoc-gen-grpc="${_GRPC_CPP_PLUGIN_EXECUTABLE}"
        "${holpaca_proto_file}"
      DEPENDS "${holpaca_proto_file}")

add_library(holpaca_stage
  ${holpaca_proto_hdrs}
  ${holpaca_grpc_hdrs}
  ./data-plane/Stage.h
  ./data-plane/CacheAllocator.h
  ${holpaca_proto_srcs}
  ${holpaca_grpc_srcs}
  ./data-plane/Stage.cpp
  ./data-plane/CacheAllocator.cpp
)

add_dependencies(holpaca_stage thrift_generated_files)
target_link_libraries(holpaca_stage PUBLIC
  cachelib_allocator
  ${_REFLECTION}
  ${_GRPC_GRPCPP}
  ${_PROTOBUF_LIBPROTOBUF}
  flows::flows
)

install(TARGETS holpaca_stage
        EXPORT cachelib-exports
        DESTINATION ${LIB_INSTALL_DIR} )

#add_subdirectory(control-plane)