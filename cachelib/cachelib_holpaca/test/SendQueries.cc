#include "../PoolCacheProxy.h"

using namespace facebook::cachelib;

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <address>" << std::endl;
  }
  PoolCacheProxy poolCacheProxy;
  poolCacheProxy.connect(argv[1]);
  // if (!poolCacheProxy.isConnected()) {
  //   std::cout << "Failed to connect to the server" << std::endl;
  //   return 1;
  // }

  std::string key = "key";
  std::string value = "value";
  poolCacheProxy.put(key, value);
  value = poolCacheProxy.get(key);
  std::cout << value << std::endl;

  return 0;
}
