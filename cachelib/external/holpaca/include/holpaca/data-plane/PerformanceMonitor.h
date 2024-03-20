#pragma once

// #include <Shards/Shards.h>

#include <atomic>
#include <map>
#include <memory>

namespace holpaca {

class PerformanceMonitor {
  // std::atomic<std::shared_ptr<Shards>> m_shards;
  std::atomic_uint64_t m_totalSize{0};
  std::atomic_uint64_t m_totalLookups{0};
  std::atomic_uint64_t m_totalHits{0};

 public:
  PerformanceMonitor();
  void reset();
  void registerLookup(std::string& key, bool hit);
  void registerEviction(std::string& key, size_t size);
  void registerResize(size_t newSize);

  std::map<uint64_t, double> getMRC();
  uint64_t getTotalSize() const;
  uint64_t getTotalLookups() const;
  uint64_t getTotalHits() const;
};
} // namespace holpaca
