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
#include <mutex>

using grpc::ServerContext;
using namespace holpaca;
using holpaca::common::Cache;

namespace holpaca::data_plane {
    class CacheServer : public virtual Cache, public virtual holpaca::cache::Service {
        private:
            // remote calls
            grpc::Status GetStatus(ServerContext* context, const StatusRequest* request, holpaca::Status* response) override final {

                auto status = this->getStatus();

                if (request->ids().empty()) {
                    for (auto const& [id, value] : status) {
                        holpaca::SubStatus ss;
                        ss.set_usedmem(value.usedMem);
                        ss.set_freemem(value.freeMem);
                        ss.set_hits(value.hits);
                        ss.set_lookups(value.lookups);
                        ss.set_evictions(value.evictions);
                        for(auto const& [cid, tailAccesses] : value.tailAccesses) {
                            (*ss.mutable_tailaccesses())[cid] = tailAccesses;
                        }
                        ss.set_isactive(value.isActive);
                        (*response->mutable_subs())[id] = ss;
                    }
                } else {
                    for (auto& id : request->ids()) {
                        holpaca::SubStatus ss;
                        ss.set_usedmem(status[id].usedMem);
                        ss.set_freemem(status[id].freeMem);
                        ss.set_hits(status[id].hits);
                        ss.set_lookups(status[id].lookups);
                        for(auto const& [cid, tailAccesses] : status[id].tailAccesses) {
                            (*ss.mutable_tailaccesses())[cid] = tailAccesses;
                        }
                        ss.set_isactive(status[id].isActive);
                        (*response->mutable_subs())[id] = ss;
                    }
                }


                return grpc::Status::OK;
            }

            grpc::Status Resize(ServerContext* context, const ResizeRequest* request, ResizeResponse* response) override final {
                this->resize(request->src(), request->dst(), request->delta());

                return grpc::Status::OK;
            }

            grpc::Status SingleResize(ServerContext* context, const SingleResizeRequest* request, ResizeResponse* response) override final {
                this->resize(request->target(), request->newsize());

                return grpc::Status::OK;
            }

            grpc::Status GetSize(ServerContext* context, const Empty* request, Size* response) override final {
                response->set_value(this->size());

                return grpc::Status::OK;
            }
    };
}

