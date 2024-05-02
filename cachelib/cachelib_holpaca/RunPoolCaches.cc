#include "CacheLibHolpaca.h"
#include "cachelib/allocator/CacheAllocator.h"
#include "holpaca/data-plane/StageConfig.h"

using namespace facebook::cachelib;

int main(int argc, char** argv) {
  if (argc < 4) {
    std::cerr << "Usage: " << argv[0]
              << " <pool-resizer=on|off> <pool-optimizer=on|off> "
                 "<cachelib-instance-size>"
                 "<controller-registration-ip> [<pool-name> "
                 "<pool-relative-size> <stage-ip> ] "
              << std::endl;
    return 1;
  }

  uint32_t total_cache_size = std::stoul(argv[3]);
  std::vector<PoolCache<Lru2QAllocator>::PoolConfig> poolConfigs;

  for (int i = 5; i < argc; i += 3) {
    holpaca::StageConfig stageConfig;
    stageConfig.setLogger("/tmp/" + std::string(argv[i]) + ".log")
        .setRegistrationAddress(argv[4])
        .setServiceAddress(argv[i + 2]);
    poolConfigs.emplace_back(PoolCache<Lru2QAllocator>::PoolConfig{
        argv[i], std::stoul(argv[i + 1]), stageConfig});
  }

  CacheLibHolpaca<Lru2QAllocator> cache(total_cache_size, argv[1] == "on",
                                        argv[2] == "on", poolConfigs);

  while (1) {
    std::this_thread::sleep_for(std::chrono::seconds(10));
  }

  // Initialize cachelib
}