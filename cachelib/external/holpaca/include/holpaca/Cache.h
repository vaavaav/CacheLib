#pragma once
#include <cstdint>
#include <map>

namespace holpaca {
struct Status {
  uint64_t maxSize;
  uint64_t usedSize;
  uint32_t hits;
  uint32_t lookups;
  std::map<uint64_t, uint32_t> tailAccesses;
  std::map<uint64_t, double> mrc;
};

class Cache {
 public:
  virtual void resize(size_t newSize) = 0;
  virtual Status getStatus() = 0;
};

} // namespace holpaca