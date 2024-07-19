#pragma once

#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>

#include <memory>
#include <thread>

#include "../Cache.h"
#include "../protos/Holpaca.grpc.pb.h"
#include "../protos/Holpaca.pb.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class Stage : public ::holpaca::Stage::Service {
  std::string m_address;
  std::shared_ptr<grpc::Server> m_server;
  Cache* m_cache;
  std::unique_ptr<::holpaca::Controller::Stub> m_controllerStub;
  std::thread m_serverThread;

 public:
  Stage(Cache* const cache, std::string address, std::string controllerAddress);
  ~Stage();
  grpc::Status GetStatus(grpc::ServerContext* context,
                         const ::holpaca::GetStatusRequest* request,
                         ::holpaca::GetStatusResponse* response) override final;
  grpc::Status Resize(grpc::ServerContext* context,
                      const ::holpaca::ResizeRequest* request,
                      ::holpaca::ResizeResponse* response) override final;
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook