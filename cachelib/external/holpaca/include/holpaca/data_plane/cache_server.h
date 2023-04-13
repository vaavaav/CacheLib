#pragma once
#include <holpaca/common/cache.h>
#include <holpaca/config.h>
#include <holpaca/common/status.h>
#include <memory>
#include <map>

#include <grpc/grpc.h>
#include <grpc++/server.h>
#include <grpc++/server_context.h>
#include <holpaca/protos/holpaca.grpc.pb.h>
#include <holpaca/protos/holpaca.pb.h>

using grpc::ServerContext;
using namespace holpaca;
using holpaca::common::Cache;

namespace holpaca::data_plane {
    class CacheServer : public virtual Cache, public virtual holpaca::cache::Service {
        private:
            // remote calls
            grpc::Status GetStatus(ServerContext* context, const StatusRequest* request, holpaca::Status* response) override final {

                auto status = this->getStatus();

                for (auto& id : request->ids()) {
                    holpaca::SubStatus ss;
                    ss.set_usedmem(status[id].usedMem);
                    ss.set_freemem(status[id].freeMem);
                    ss.set_hits(status[id].hits);
                    ss.set_lookups(status[id].lookups);
                    (*response->mutable_subs())[id] = ss;
                }

                return grpc::Status::OK;
            }

            grpc::Status Resize(ServerContext* context, const ResizeRequest* request, ResizeResponse* response) override final {
                this->resize(request->src(), request->dst(), request->delta());

                return grpc::Status::OK;
            }
    };
}

