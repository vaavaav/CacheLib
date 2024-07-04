#include "CacheLibHolpaca.h"

#include "cachelib/allocator/HitsPerSlabStrategy.h"

namespace facebook::cachelib {
template <typename CacheTrait>
CacheLibHolpaca<CacheTrait>::CacheLibHolpaca(
    size_t cacheSize,
    bool poolResizer,
    bool poolOptimizer,
    std::vector<typename PoolCache<CacheTrait>::PoolConfig> pools) {
  typename CacheTrait::Config config;
  config.setCacheSize(cacheSize)
      .setCacheName("CacheLibHolpaca")
      .setAccessConfig({25, 10});
  if (poolResizer) {
    config.enablePoolResizing(
        std::make_shared<HitsPerSlabStrategy>(
            HitsPerSlabStrategy::Config(0.25, static_cast<unsigned int>(1))),
        std::chrono::milliseconds(1000), 1);
  }
  if (poolOptimizer) {
    config.enablePoolOptimizer(std::make_shared<PoolOptimizeStrategy>(),
                               std::chrono::seconds(1), std::chrono::seconds(1),
                               0);
  }
  m_cache = std::make_shared<CacheTrait>(config);
  for (const auto& pool : pools) {
    if (!pool.stageConfig.isValid()) {
      throw std::runtime_error("Invalid stage config for pool " + pool.name);
    }
    m_pools[pool.name] = std::make_shared<PoolCache<CacheTrait>>(m_cache, pool);
  }
};

template <typename CacheTrait>
std::shared_ptr<PoolCache<CacheTrait>> CacheLibHolpaca<CacheTrait>::operator[](
    const std::string& name) {
  return m_pools[name];
}

template class CacheLibHolpaca<LruAllocator>;
// template class CacheLibHolpaca<LruAllocatorSpinBuckets>;
// template class CacheLibHolpaca<Lru2QAllocator>;
// template class CacheLibHolpaca<TinyLFUAllocator>;

} // namespace facebook::cachelib