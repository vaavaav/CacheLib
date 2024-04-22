#include "cachelib.h"

#include "core/db_factory.h"
using std::cout;
using std::endl;

#include <sstream>
#include <unordered_map>

#include "cachelib/allocator/CacheAllocator.h"
#include "cachelib/allocator/HitsPerSlabStrategy.h"
#include "cachelib/allocator/MarginalHitsOptimizeStrategy.h"
#include "cachelib/allocator/PoolOptimizeStrategy.h"

namespace {
const std::string PROP_CACHE_SIZE = "cachelib.cachesize";
const std::string PROP_CACHE_SIZE_DEFAULT =
    std::to_string(1 * 1024 * 1024 * 1024);

const std::string PROP_CACHE_ALLOCATOR = "cachelib.cache_allocator";
const std::string PROP_CACHE_ALLOCATOR_DEFAULT = "lru";

const std::string PROP_CACHE_TRACKER = "cachelib.tracker";
const std::string PROP_CACHE_TRACKER_DEFAULT = "/tmp/tracker.log";

const std::string PROP_POOL_OPTIMIZER = "cachelib.pool_optimizer";
const std::string PROP_POOL_OPTIMIZER_DEFAULT = "off";

const std::string PROP_POOL_OPTIMIZER_PERIODICITY =
    "cachelib.pool_optimizer_periodicity";
const std::string PROP_POOL_OPTIMIZER_PERIODICITY_DEFAULT = "0";

const std::string PROP_TAIL_HITS_TRACKING = "cachelib.tail_hits_tracking";
const std::string PROP_TAIL_HITS_TRACKING_DEFAULT = "off";

const std::string PROP_POOL_RESIZER = "cachelib.pool_resizer";
const std::string PROP_POOL_RESIZER_DEFAULT = "off";

const std::string PROP_POOL_RESIZER_PERIODICITY =
    "cachelib.pool_resizer_periodicity";
const std::string PROP_POOL_RESIZER_PERIODICITY_DEFAULT = "100";
} // namespace

namespace ycsbc {

std::mutex CacheLib::mutex_;
RocksDB CacheLib::rocksdb_;
std::unique_ptr<CacheLib::CacheLibAllocator> CacheLib::cache_ = nullptr;
int CacheLib::ref_cnt_ = 0;
thread_local int CacheLib::threadId_;
std::vector<facebook::cachelib::PoolId> CacheLib::pools_;

void CacheLib::Init() {
  rocksdb_.Init();
  std::lock_guard<std::mutex> lock(mutex_);
  if (cache_ == nullptr) {
    CacheLib::CacheLibAllocator::Config config;
    config
        .setCacheSize(std::stol(
            props_->GetProperty(PROP_CACHE_SIZE, PROP_CACHE_SIZE_DEFAULT)))
        .setCacheName("YCSBenchmark")
        .setAccessConfig({25, 15});
    if (props_->GetProperty(PROP_TAIL_HITS_TRACKING,
                            PROP_TAIL_HITS_TRACKING_DEFAULT) == "on") {
      config.enableTailHitsTracking();
      if (props_->GetProperty(PROP_POOL_OPTIMIZER,
                              PROP_POOL_OPTIMIZER_DEFAULT) == "on") {
        config.enablePoolOptimizer(
            std::make_shared<
                facebook::cachelib::MarginalHitsOptimizeStrategy>(),
            std::chrono::seconds(1),
            std::chrono::seconds(1),
            0);
      }
    }
    if (props_->GetProperty(PROP_POOL_RESIZER, PROP_POOL_RESIZER_DEFAULT) ==
        "on") {
      config.enablePoolResizing(
          std::make_shared<facebook::cachelib::HitsPerSlabStrategy>(
              facebook::cachelib::HitsPerSlabStrategy::Config(
                  0.25, static_cast<unsigned int>(1))),
          std::chrono::milliseconds(std::stoi(
              props_->GetProperty(PROP_POOL_RESIZER_PERIODICITY,
                                  PROP_POOL_RESIZER_PERIODICITY_DEFAULT))),
          1);
    }
    if (props_->GetProperty(PROP_HOLPACA, PROP_HOLPACA_DEFAULT) == "on") {
      config.enableHolpaca(
          std::chrono::milliseconds(std::stoi(props_->GetProperty(
              PROP_HOLPACA_PERIODICITY, PROP_HOLPACA_PERIODICITY_DEFAULT))));
    }
    if (props_->ContainsKey(PROP_CACHE_TRACKER)) {
      config.enableTracker(
          std::chrono::milliseconds(1000),
          props_->GetProperty(PROP_CACHE_TRACKER, PROP_CACHE_TRACKER_DEFAULT));
    }
    config.validate();

    cache_ = std::make_unique<CacheLib::CacheLibAllocator>(config);
    int threads = std::stoi(props_->GetProperty("threadcount", "1"));
    for (int i = 0; i < threads; i++) {
      pools_.push_back(cache_->addPool(
          std::to_string(i),
          cache_->getCacheMemoryStats().ramCacheSize * 1.0f / threads));
    }
  }
}

DB::Status CacheLib::Read(const std::string& table,
                          const std::string& key,
                          const std::vector<std::string>* fields,
                          std::vector<Field>& result) {
  auto handle = cache_->find(key);

  if (handle == nullptr) {
    if (rocksdb_.Read(table, key, fields, result) == kOK) {
      std::string data = result.front().value;
      auto new_handle = cache_->allocate(pools_[threadId_], key, data.size());

      if (new_handle) {
        std::memcpy(new_handle->getMemory(), data.data(), data.size());
        cache_->insertOrReplace(new_handle);
      } else {
        return kError;
      }
    }
    return kNotFound;
  }
  volatile auto data = handle->getMemory();

  return kOK;
}

DB::Status CacheLib::Scan(const std::string& table,
                          const std::string& key,
                          long len,
                          const std::vector<std::string>* fields,
                          std::vector<std::vector<Field>>& result) {
  // seek key
  auto itr = cache_->begin();
  for (; itr != cache_->end(); ++itr) {
    if (key.compare(itr->getKey()) == 0) {
      break;
    }
  }

  for (long i = 0; i < len; ++i) {
    if (itr == cache_->end()) {
      break;
    }
    const void* item = itr->getMemory();
    ++itr;
  }

  return kOK;
}

DB::Status CacheLib::Update(const std::string& table,
                            const std::string& key,
                            std::vector<Field>& values) {
  std::string data = values.front().value;

  auto handle = cache_->allocate(pools_[threadId_], key, data.size());
  if (handle) {
    std::memcpy(handle->getMemory(), data.data(), data.size());
    auto item = cache_->insertOrReplace(handle);
    if (item == nullptr) {
      rocksdb_.Update(table, key, values);
    }
    return item == nullptr ? kNotFound : kOK;
  } else {
    return kError;
  }
}

DB::Status CacheLib::Insert(const std::string& table,
                            const std::string& key,
                            std::vector<Field>& values) {
  std::string data = values.front().value;
  auto new_handle = cache_->allocate(pools_[threadId_], key, data.size());

  if (new_handle) {
    std::memcpy(new_handle->getMemory(), data.data(), data.size());
    cache_->insertOrReplace(new_handle);
  } else {
    return kError;
  }

  return rocksdb_.Insert(table, key, values);
}

DB::Status CacheLib::Delete(const std::string& table, const std::string& key) {
  return cache_->remove(key) == CacheLib::CacheLibAllocator::RemoveRes::kSuccess
             ? kOK
             : kNotFound;
}

DB* NewCacheLib() { return new CacheLib; }

const bool registered = DBFactory::RegisterDB("cachelib", NewCacheLib);

void CacheLib::SetThreadId(int id) { threadId_ = id; }

} // namespace ycsbc
