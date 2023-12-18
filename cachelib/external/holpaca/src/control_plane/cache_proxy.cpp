#include <holpaca/control_plane/cache_proxy.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace holpaca::control_plane {
CacheProxy::CacheProxy(char const* address) {
  auto s = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  cache = holpaca::cache::NewStub(s);
}

void CacheProxy::resize(Id srcPool, Id dstPool, size_t delta) {
  ClientContext context;
  ResizeRequest request;
  request.set_src(srcPool);
  request.set_dst(dstPool);
  request.set_delta(delta);

  ResizeResponse response;

  cache->Resize(&context, request, &response);
}

void CacheProxy::resize(Id target, size_t newSize) {
  ClientContext context;
  SingleResizeRequest request;
  request.set_target(target);
  request.set_newsize(newSize);

  ResizeResponse response;

  cache->SingleResize(&context, request, &response);
}

holpaca::common::Status CacheProxy::getStatus() {
  ClientContext context;
  StatusRequest request;
  request.clear_ids();
  holpaca::Status response;

  auto r =  cache->GetStatus(&context, request, &response);

  holpaca::common::Status result{};

  for (auto const& [id, value] : response.subs()) {
    std::map<uint64_t, uint32_t> tailAccesses;
    std::map<uint32_t, double> mrc;
    for (auto const& [cid, cs] : value.tailaccesses()) {
      tailAccesses[cid] = cs;
    }
    for (auto const& [cs, mr] : value.mrc()) {
      mrc[cs] = mr;
    }
    result[id] = {value.usedmem(), value.freemem(), value.hits(),
                  value.lookups(), value.evictions(), tailAccesses, mrc, value.meanobjectsize(), value.isactive()};
  }

  return result;
}

size_t CacheProxy::size() {
  ClientContext context;
  Empty request;
  Size response;

  cache->GetSize(&context, request, &response);

  return response.value();
}

} // namespace holpaca::control_plane