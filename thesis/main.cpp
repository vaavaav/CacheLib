/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "cachelib/allocator/CacheAllocator.h"
#include "cachelib/allocator/PoolOptimizeStrategy.h"
#include "folly/init/Init.h"

#include <zmq.hpp>
#include "constants.hpp"
#include <thread>
#include <chrono>
#include <sstream>

namespace facebook {
namespace cachelib_examples {
using Cache = cachelib::LruAllocator; // or Lru2QAllocator, or TinyLFUAllocator
using CacheConfig = typename Cache::Config;
using CacheKey = typename Cache::Key;
using CacheReadHandle = typename Cache::ReadHandle;

// Global cache object and a default cache pool
std::unique_ptr<Cache> gCache_;
cachelib::PoolId pools[3];

// Global sockets 
zmq::context_t singleton_context;

void poolDataPlane_ish(std::string pool_id_str, int init_size) {
    zmq::socket_t metrics_sock (singleton_context, zmq::socket_type::push);
    metrics_sock.connect("ipc://" + metrics_socket_address);

    auto pool_id = gCache_->addPool(pool_id_str, init_size);
    
    while(true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        auto pool_stats = gCache_->getPoolStats(pool_id);
        std::ostringstream message_ss;
        message_ss << pool_id_str << " " << pool_stats.poolSize << " " << pool_stats.numPoolGetHits;
        zmq::message_t message (message_ss.str());
        metrics_sock.send(message); //deprecated
    } 
    metrics_sock.close();
}

void initializeCache() {
  CacheConfig config;
  config
      .setCacheSize(1 * 1024 * 1024 * 1024) // 1GB
      .setCacheName("My Use Case")
      .setAccessConfig(
          {25 /* bucket power */, 10 /* lock power */}) // assuming caching 20
                                                        // million items
      .enablePoolOptimizer(std::make_shared<cachelib::PoolOptimizeStrategy>(), std::chrono::seconds(1), std::chrono::seconds(1), 0)
      .validate(); // will throw if bad config
  gCache_ = std::make_unique<Cache>(config);
}

void destroyCache() { gCache_.reset(); }

CacheReadHandle get(CacheKey key) { return gCache_->find(key); }

bool put(CacheKey key, const std::string& value) {
  auto handle = gCache_->allocate(pools[0], key, value.size());
  if (!handle) {
    return false; // cache may fail to evict due to too many pending writes
  }
  std::memcpy(handle->getMemory(), value.data(), value.size());
  gCache_->insertOrReplace(handle);
  return true;
}
} // namespace cachelib_examples
} // namespace facebook

using namespace facebook::cachelib_examples;

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  initializeCache();

  int number_of_instances { sizeof instances / sizeof instances[0] };
  std::thread data_plane_stages[number_of_instances]; 
  for (int i = 0; i < number_of_instances; i++) {
    data_plane_stages[i] = std::thread(poolDataPlane_ish, instances[i], (int) gCache_->getCacheMemoryStats().cacheSize/number_of_instances);
  }
  data_plane_stages[0].join();

  destroyCache();
}
