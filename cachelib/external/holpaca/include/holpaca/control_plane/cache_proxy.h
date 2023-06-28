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
#include <spdlog/spdlog.h>

using grpc::ClientContext;
using grpc::Channel;

namespace holpaca::control_plane {
    class CacheProxy final : public virtual holpaca::common::Cache {
        std::unique_ptr<holpaca::cache::Stub> cache;
        std::shared_ptr<spdlog::logger> logger;

        public:
            CacheProxy(char const* address); 

            void resize(Id srcPool, Id dstPool, size_t delta) override final;
            void resize(Id target, size_t newSize) override final;
            size_t size() override final;

            holpaca::common::Status getStatus() override final;
    };
}