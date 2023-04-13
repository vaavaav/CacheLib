#pragma once
#include <grpcpp/grpcpp.h>
#include <grpcpp/channel.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/client_context.h>
#include <grpcpp/security/credentials.h>
#include <holpaca/config.h>
#include <holpaca/common/cache.h>
#include <holpaca/protos/holpaca.pb.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <thread>
#include <map>

using grpc::ClientContext;
using grpc::Channel;

namespace holpaca::control_plane {
    class CacheProxy final : public virtual holpaca::common::Cache {
        std::unique_ptr<holpaca::cache::Stub> cache;

        public:
            CacheProxy(char const* address = holpaca::data_plane::config::stage_address); 

            void resize(Id srcPool, Id dstPool, size_t delta) override final;

            holpaca::common::Status getStatus() override final;
            
    };
}