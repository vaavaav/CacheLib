#include <holpaca/control_plane/cache_proxy.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace holpaca::control_plane {
CacheProxy::CacheProxy(char const* address) {
  std::cout << "1" << std::endl;
  auto s = grpc::CreateChannel(address, grpc::InsecureChannelCredentials());
  std::cout << "2" << std::endl;
  cache = holpaca::cache::NewStub(s);
  std::cout << "3" << std::endl;
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

holpaca::common::Status CacheProxy::getStatus() {
  ClientContext context;
  StatusRequest request;
  request.clear_ids();
  holpaca::Status response;

  cache->GetStatus(&context, request, &response);

  holpaca::common::Status result{};

  for (auto const& [id, value] : response.subs()) {
    result[id] = {value.usedmem(), value.freemem(), value.hits(),
                  value.lookups()};
  }

  return result;
}
} // namespace holpaca::control_plane