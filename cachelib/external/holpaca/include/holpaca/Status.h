#pragma once
#include <map>

namespace holpaca {
struct Status {
  uint64_t maxSize;
  uint32_t hits;
  uint32_t lookups;
  std::map<uint64_t, uint32_t> tailAccesses;
  std::map<uint64_t, double> mrc;
};

} // namespace holpaca