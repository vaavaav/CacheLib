#pragma once
#include <flows/Flows.h>

#include "Stage.h"
#include "cachelib/allocator/CacheAllocator.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
template <typename CacheTrait>
class CacheAllocator : public Cache,
                       public ::facebook::cachelib::CacheAllocator<CacheTrait> {
  std::shared_ptr<Stage> m_stage;

  void resize(std::unordered_map<int32_t, uint64_t> newSizes) override final;
  std::unordered_map<int32_t, PoolStatus> getStatus() override final;

  std::unordered_map<int32_t, std::shared_ptr<Flows>> m_flows;
  using Super = ::facebook::cachelib::CacheAllocator<CacheTrait>;

 public:
  using Config = ::facebook::cachelib::CacheAllocatorConfig<Super>;

  CacheAllocator(Config config,
                 std::string address,
                 std::string controllerAddress);

  PoolId addPool(std::string name, size_t size);

  bool put(PoolId id, const std::string& key, const std::string& value);

  std::string get(PoolId id, const std::string& key);

  friend class Stage;
};

using LruAllocator = CacheAllocator<LruCacheTrait>;
using LruWithSpinAllocator = CacheAllocator<LruCacheWithSpinBucketsTrait>;
using Lru2QAllocator = CacheAllocator<Lru2QCacheTrait>;
using TinyLFUAllocator = CacheAllocator<TinyLFUCacheTrait>;

} // namespace holpaca
} // namespace cachelib
} // namespace facebook