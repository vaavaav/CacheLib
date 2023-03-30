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

#include "../../../protos/holpaca.pb.h"
#include "../../../protos/holpaca.grpc.pb.h"

using namespace holpaca::data_plane;

namespace holpaca::control_plane {
    class Controller final : public Holpaca::Controller::Service {
      public:
        explicit Controller() {}

    };
}

int main() {
  return 0;
}