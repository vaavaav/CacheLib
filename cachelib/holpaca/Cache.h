#pragma once
#include <cstdint>
#include <map>
#include <unordered_map>

namespace facebook {
namespace cachelib {
namespace holpaca {
struct PoolStatus {
  uint64_t maxSize;
  uint64_t usedSize;
  std::map<uint64_t, uint32_t> tailAccesses;
  std::map<uint64_t, float> mrc;
};

class Cache {
 public:
  virtual void resize(std::unordered_map<int32_t, uint64_t> newSizes) = 0;
  virtual std::unordered_map<int32_t, PoolStatus> getStatus() = 0;
};

} // namespace holpaca
} // namespace cachelib
} // namespace facebook