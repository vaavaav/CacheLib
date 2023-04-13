#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage.h>
#include <memory>
#include <holpaca/config.h>
#include <unordered_map>

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

using namespace holpaca::data_plane;

namespace holpaca::control_plane {
    class Controller {
      public:
        explicit Controller() {}

    };
}