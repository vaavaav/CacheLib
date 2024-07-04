#pragma once
#include <cachelib/allocator/CacheAllocator.h>
#include <flows/Flows.h>
#include <holpaca/data-plane/Stage.h>

#include <shared_mutex>

namespace facebook::cachelib {
template <typename CacheTrait>
class PoolCache : public holpaca::Stage {
  uint64_t m_hits{0};
  uint64_t m_lookups{0};
  std::shared_mutex m_mutex;
  std::shared_ptr<Flows> m_flows;
  std::shared_ptr<CacheTrait> m_cachelibInstance;
  PoolId m_poolId;

 public:
  struct PoolConfig {
    std::string name;
    size_t size;
    holpaca::StageConfig stageConfig;
  };
  PoolCache(std::shared_ptr<CacheTrait> cachelibInstance,
            PoolConfig poolConfig);
  std::string get(std::string& key) override final;
  bool put(std::string& key, std::string& value) override final;
  bool remove(std::string& key) override final;
  void resize(size_t newSize) override final;
  holpaca::Status getStatus() override final;
};
} // namespace facebook::cachelib
