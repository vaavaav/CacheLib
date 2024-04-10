#pragma once

#include <holpaca/Cache.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>

namespace holpaca
{
  class StageServer : public holpaca::stage::Service
  {
    Cache *m_cache;

  public:
    StageServer(Cache *cache);

    grpc::Status GetStatus(grpc::ServerContext *context,
                           const StatusRequest *request,
                           StatusResponse *response) override final;
    grpc::Status Resize(grpc::ServerContext *context,
                        const ResizeRequest *request,
                        ResizeResponse *response) override final;

    // Stage Control Interface
  };
} // namespace holpaca