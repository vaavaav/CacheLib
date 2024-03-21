#pragma once
#include <grpc/grpc.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <holpaca/Cache.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>
#include <spdlog/spdlog.h>

using grpc::Channel;
using grpc::ClientContext;

namespace holpaca {
class StageProxy : public Cache {
  std::shared_ptr<grpc::Channel> m_channel;
  std::unique_ptr<holpaca::stage::Stub> m_stage;

 public:
  StageProxy() = default;
  StageProxy& connect(const std::string& address);
  bool isConnected() const;
  bool isValid() const;

  // Cache Interface
  void resize(size_t newSize) override final;
  Status getStatus() override final;
};
} // namespace holpaca