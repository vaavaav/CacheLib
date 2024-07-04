#include "PoolCache.h"

namespace facebook::cachelib {
template <typename CacheTrait>
PoolCache<CacheTrait>::PoolCache(std::shared_ptr<CacheTrait> cachelibInstance,
                                 PoolConfig poolConfig)
    : Stage(poolConfig.stageConfig) {
  if (cachelibInstance == nullptr) {
    throw std::invalid_argument("cachelib instance cannot be null");
  }
  m_cachelibInstance = cachelibInstance;
  m_poolId = cachelibInstance->addPool(poolConfig.name, poolConfig.size);
  m_flows = std::make_shared<Flows>(0); // maxItems is not working yet
}

template <typename CacheTrait>
std::string PoolCache<CacheTrait>::get(std::string& key) {
  auto handle = m_cachelibInstance->find(key);
  std::unique_lock<std::shared_mutex> lock(m_mutex);
  m_lookups++;
  m_flows->read(key);
  if (handle != nullptr) {
    m_hits++;
    return std::string(reinterpret_cast<const char*>(handle->getMemory()),
                       handle->getSize());
  }
  return "";
}

template <typename CacheTrait>
bool PoolCache<CacheTrait>::put(std::string& key, std::string& value) {
  auto handle = m_cachelibInstance->allocate(m_poolId, key, value.size());
  if (!handle) {
    return false;
  }
  m_flows->write(key, key.size() + value.size());
  std::memcpy(handle->getMemory(), value.data(), value.size());
  m_cachelibInstance->insertOrReplace(handle);
  return true;
}

template <typename CacheTrait>
bool PoolCache<CacheTrait>::remove(std::string& key) {
  // TODO: this can remove keys from other pools as well...
  return m_cachelibInstance->remove(key) == CacheTrait::RemoveRes::kSuccess;
}

template <typename CacheTrait>
void PoolCache<CacheTrait>::resize(size_t newSize) {
  auto oldSize = m_cachelibInstance->getPoolStats(m_poolId).poolSize;
  if (newSize > oldSize) {
    m_cachelibInstance->growPool(m_poolId, newSize - oldSize);
  } else {
    m_cachelibInstance->shrinkPool(m_poolId, oldSize - newSize);
  }
}

template <typename CacheTrait>
holpaca::Status PoolCache<CacheTrait>::getStatus() {
  auto stats = m_cachelibInstance->getPoolStats(m_poolId);
  auto mrc = m_flows->bmrc();
  std::map<uint64_t, double> mrc2(mrc.begin(), mrc.end());

  holpaca::Status status{
      stats.poolSize, stats.poolUsableSize, m_hits, m_lookups, {}, mrc2};
  return status;
}
template class PoolCache<LruAllocator>;
// template class PoolCache<LruAllocatorSpinBuckets>;
// template class PoolCache<Lru2QAllocator>;
// template class PoolCache<TinyLFUAllocator>;
} // namespace facebook::cachelib