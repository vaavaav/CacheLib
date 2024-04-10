#include "PoolStage.h"

namespace facebook::cachelib {
template <typename CacheTrait>
PoolCache<CacheTrait>::PoolCache(std::string name,
                                 size_t size,
                                 std::shared_ptr<CacheTrait> cachelibInstance) {
  if (cachelibInstance == nullptr) {
    throw std::invalid_argument("cachelib instance cannot be null");
  }
  m_poolId = cachelibInstance->addPool(name, size);
  m_shards = std::shared_ptr<Shards>(Shards::fixedSize(0.001, 32000, 1));
}

template <typename CacheTrait>
std::string PoolCache<CacheTrait>::get(std::string& key) {
  auto item = m_cachelibInstance->get(m_poolId, key);
  std::unique_lock<std::shared_mutex> lock(m_mutex);
  m_lookups++;
  m_shards->feed(key);
  if (item.has_value()) {
    m_hits++;
    return item.value();
  }
  return "";
}

template <typename CacheTrait>
bool PoolCache<CacheTrait>::put(std::string& key, std::string& value) {
  auto handle = m_cachelibInstance->allocate(m_poolId, key, value.size());
  if (!handle) {
    return false;
  }
  std::memcpy(handle->getMemory(), value.data(), value.size());
  m_cachelibInstance->insertOrReplace(handle);
  return true;
}

template <typename CacheTrait>
bool PoolCache<CacheTrait>::remove(std::string& key) {
  return m_cachelibInstance->remove(m_poolId, key);
}

template <typename CacheTrait>
void PoolCache<CacheTrait>::resize(size_t newSize) {
  m_cachelibInstance->resizePool(m_poolId, newSize);
}

template <typename CacheTrait>
holpaca::Status PoolCache<CacheTrait>::getStatus() {
  auto stats = m_cachelibInstance->getPoolStats(m_poolId);
  holpaca::Status status{
      stats.poolSize, stats.poolUsableSize, 0, m_hits, m_lookups, {},
      m_shards->mrc()};
  return status;
}

} // namespace facebook::cachelib