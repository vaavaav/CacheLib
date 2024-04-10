#include "PoolCache.h"

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
  auto handle = m_cachelibInstance->find(key);
  std::unique_lock<std::shared_mutex> lock(m_mutex);
  m_lookups++;
  m_shards->feed(key);
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
  holpaca::Status status{
      stats.poolSize, stats.poolUsableSize, m_hits, m_lookups, {},
      m_shards->mrc()};
  return status;
}
template class PoolCache<LruAllocator>;
template class PoolCache<LruAllocatorSpinBuckets>;
template class PoolCache<Lru2QAllocator>;
template class PoolCache<TinyLFUAllocator>;
} // namespace facebook::cachelib