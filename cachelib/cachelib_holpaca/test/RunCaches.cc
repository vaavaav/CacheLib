#include "../PoolCache.h"
#include "cachelib/allocator/CacheAllocator.h"
#include "cachelib/allocator/HitsPerSlabStrategy.h"
#include "holpaca/data-plane/Stage.h"
#include "holpaca/data-plane/StageConfig.h"

using namespace facebook::cachelib;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr
        << "Usage: " << argv[0]
        << " <pool-optimizer=on|off> <pool-resizer=on|off> <cachelib-size>"
           "<registration-ip> [<pool-name> "
           "<partition-rel-size> <ip> ] "
        << std::endl;
    return 1;
  }

  uint32_t total_cache_size = std::stoul(argv[3]);

  using Cache = Lru2QAllocator;
  using CacheConfig = typename Cache::Config;

  CacheConfig config;
  config.setCacheSize(total_cache_size)
      .setCacheName("CacheLibHolpaca")
      .setAccessConfig({25, 10});
  if (argv[1] == "on") {
    config.enablePoolOptimizer(std::make_shared<PoolOptimizeStrategy>(),
                               std::chrono::seconds(1), std::chrono::seconds(1),
                               0);
  }
  if (argv[2] == "on") {
    config.enablePoolResizing(
        std::make_shared<HitsPerSlabStrategy>(
            HitsPerSlabStrategy::Config(0.25, static_cast<unsigned int>(1))),
        std::chrono::milliseconds(1000), 1);
  }
  config.validate();

  auto cache = std::make_shared<Cache>(config);
  std::vector<std::shared_ptr<holpaca::Stage>> stages;

  for (int i = 5; i < argc; i += 3) {
    auto pc = new PoolCache<Cache>(
        argv[i],
        std::stof(argv[i + 1]) * cache->getCacheMemoryStats().ramCacheSize,
        cache);
    holpaca::StageConfig stage_config;
    stage_config.setLogger("/tmp/" + std::string(argv[i]) + ".log")
        .setRegistrationAddress(argv[4])
        .setServiceAddress(argv[i + 2])
        .setCache(pc);
    if (!stage_config.isValid()) {
      std::cerr << "Invalid stage config" << std::endl;
      return 1;
    }
    stages.push_back(std::make_shared<holpaca::Stage>(stage_config));
  }

  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  // Initialize cachelib
}