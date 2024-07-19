#include "CacheAllocator.h"

#include <algorithm>
#include <vector>

namespace facebook {
namespace cachelib {
namespace holpaca {

template <typename CacheTrait>
CacheAllocator<CacheTrait>::CacheAllocator(Config config,
                                           std::string address,
                                           std::string controllerAddress)
    : ::facebook::cachelib::CacheAllocator<CacheTrait>(
          std::move(static_cast<typename Super::Config>(config))) {
  m_stage = std::make_shared<Stage>(static_cast<Cache*>(this), address,
                                    controllerAddress);
}

template <typename CacheTrait>
void CacheAllocator<CacheTrait>::resize(
    std::unordered_map<int32_t, uint64_t> newSizes) {
  // CacheLib provides a resize method based on relative (not absolute sizes)
  auto poolStats = this->getStatus();
  std::vector<std::pair<int32_t, int64_t>> sortedRelSizes; // relSizes may
                                                           // be negative
  for (const auto& [poolId, newSize] : newSizes) {
    sortedRelSizes.push_back({poolId, newSize - poolStats[poolId].maxSize});
  }

  // resizing must be done in order from the most to least downsized pool
  // else, when expanding a pool, we may not have enough memory to expand
  std::sort(sortedRelSizes.begin(), sortedRelSizes.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });

  for (auto [poolId, relSize] : sortedRelSizes) {
    if (relSize < 0) {
      // downsizing
      Super::shrinkPool(static_cast<::facebook::cachelib::PoolId>(poolId),
                        -relSize);
    } else {
      // upsizing
      Super::growPool(static_cast<::facebook::cachelib::PoolId>(poolId),
                      relSize);
    }
  }
}

template <typename CacheTrait>
std::unordered_map<int32_t, PoolStatus>
CacheAllocator<CacheTrait>::getStatus() {
  std::unordered_map<int32_t, PoolStatus> poolStatus;
  for (auto const pid : Super::getPoolIds()) {
    auto stats = Super::getPoolStats(pid);
    PoolStatus s;
    s.maxSize = stats.poolSize;
    s.usedSize = stats.poolUsableSize + stats.poolAdvisedSize;
    for (auto const& [cid, cs] : stats.cacheStats) {
      s.tailAccesses[cid] = cs.containerStat.numTailAccesses;
    }
    s.mrc = m_flows[pid]->bmrc();
    poolStatus[pid] = std::move(s);
  }
  return poolStatus;
}

template <typename CacheTrait>
PoolId CacheAllocator<CacheTrait>::addPool(std::string name, size_t size) {
  auto poolId = Super::addPool(name, size);
  m_flows[poolId] = std::make_unique<Flows>(32000);
  return poolId;
}

template <typename CacheTrait>
bool CacheAllocator<CacheTrait>::put(PoolId id,
                                     const std::string& key,
                                     const std::string& value) {
  auto handle = Super::allocate(id, key, value.size());
  if (!handle) {
    return false;
  }
  auto kkey = key;
  m_flows[id]->write(kkey, value.size());
  std::memcpy(handle->getMemory(), value.data(), value.size());
  Super::insertOrReplace(handle);
  return true;
}

template <typename CacheTrait>
std::string CacheAllocator<CacheTrait>::get(PoolId id, const std::string& key) {
  auto handle = Super::find(key);
  if (!handle) {
    return "";
  }
  auto kkey = key;
  m_flows[id]->read(kkey);
  return std::string(reinterpret_cast<const char*>(handle->getMemory()),
                     handle->getSize());
}

template class CacheAllocator<::facebook::cachelib::LruCacheTrait>;
template class CacheAllocator<
    ::facebook::cachelib::LruCacheWithSpinBucketsTrait>;
template class CacheAllocator<::facebook::cachelib::Lru2QCacheTrait>;
template class CacheAllocator<::facebook::cachelib::TinyLFUCacheTrait>;
} // namespace holpaca
} // namespace cachelib
} // namespace facebook