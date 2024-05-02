#pragma once
#include <cachelib/cachelib_holpaca/PoolCache.h>

#include <vector>

namespace facebook::cachelib {

template <typename CacheTrait>
class CacheLibHolpaca {
  std::shared_ptr<CacheTrait> m_cache;
  std::unordered_map<std::string, std::shared_ptr<PoolCache<CacheTrait>>>
      m_pools;

 public:
  CacheLibHolpaca(
      size_t cacheSize,
      bool poolResizer,
      bool poolOptimizer,
      std::vector<typename PoolCache<CacheTrait>::PoolConfig> pools);

  std::shared_ptr<PoolCache<CacheTrait>> operator[](const std::string& name);
};
} // namespace facebook::cachelib
