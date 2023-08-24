#pragma once

#include <atomic>
#include <fstream>
#include <set>

#include "cachelib/allocator/Cache.h"
#include "cachelib/allocator/RebalanceStrategy.h"
#include "cachelib/allocator/SlabReleaseStats.h"
#include "cachelib/common/PeriodicWorker.h"

namespace facebook {
namespace cachelib {

class Tracker : public PeriodicWorker {
 public:
  // @param cache                 the cache interace
  Tracker(CacheBase& cache, std::string pathToFile) : cache_(cache) {
    log.open(pathToFile);
  }

  ~Tracker() override {
    log.close();
    stop(std::chrono::milliseconds(0));
  }

 private:
  CacheBase& cache_;
  std::ofstream log;
  long time{0};

  void work() override final {
    auto pools = cache_.getActivePools();
    std::set<PoolId> pids(pools.begin(), pools.end());
    log << fmt::format("{}: ", time++);
    for (auto const& id : cache_.getPoolIds()) {
      auto pool_stats = cache_.getPoolStats(id);
      bool active = pids.find(id) != pids.end();
      log << fmt::format("pool-{} {{ usedMem={} freeMem={} }} ", id,
                         active * pool_stats.poolSize,
                         active * pool_stats.freeMemoryBytes());
    }
    log << std::endl;
  }
};
} // namespace cachelib
} // namespace facebook
