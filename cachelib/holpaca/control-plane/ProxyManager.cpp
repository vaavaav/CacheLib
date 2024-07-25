#include "cachelib/holpaca/control-plane/ProxyManager.h"

#include <iostream>
#include <mutex>

#include "cachelib/holpaca/Cache.h"

namespace facebook {
namespace cachelib {
namespace holpaca {

std::unordered_map<std::string, std::shared_ptr<Cache>>
ProxyManager::getCaches() {
  std::shared_lock<std::shared_timed_mutex> lock(m_mutex);
  std::unordered_map<std::string, std::shared_ptr<Cache>> caches;
  for (const auto& [address, proxy] : m_proxies) {
    caches[address] = proxy;
  }
  return caches;
}

grpc::Status ProxyManager::KeepAlive(grpc::ServerContext* context,
                                     const ::holpaca::KeepAliveRequest* request,
                                     ::holpaca::KeepAliveResponse* response) {
  std::lock_guard<std::shared_timed_mutex> lock(m_mutex);
  auto address = request->address();
  if (m_proxies.find(address) == m_proxies.end()) {
    m_proxies[address] = std::make_shared<CacheProxy>(
        address,
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()));
  }
  m_proxies[address]->keepAlive(
      std::chrono::duration_cast<std::chrono::nanoseconds>(
          std::chrono::steady_clock::now().time_since_epoch()));
  return grpc::Status::OK;
}

} // namespace holpaca
} // namespace cachelib
} // namespace facebook
