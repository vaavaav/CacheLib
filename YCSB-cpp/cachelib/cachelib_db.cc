#include "cachelib_db.h"
#include "core/db_factory.h"
using std::cout;
using std::endl;

#include "cachelib/allocator/PoolOptimizeStrategy.h"
#include "cachelib/allocator/HitsPerSlabStrategy.h"

namespace ycsbc {

std::mutex CacheLibDB:: mutex_;
std::unique_ptr<facebook::cachelib::Lru2QAllocator> CacheLibDB:: cache_;
facebook::cachelib::PoolId CacheLibDB:: defaultPool;

void CacheLibDB::Init() {
  std::lock_guard<std::mutex> lock(mutex_);
  cout << "init" << endl;
  facebook::cachelib::Lru2QAllocator::Config config;
  config
    .setCacheSize(1 * 1024 * 1024 * 1024)
    .setCacheName("YCSBenchmark")
    .setAccessConfig({25, 10})
    .enablePoolOptimizer(
      std::make_shared<facebook::cachelib::PoolOptimizeStrategy>(),
      std::chrono::seconds(1),
      std::chrono::seconds(1),
      0
    )
    .enablePoolResizing(
    std::make_shared<facebook::cachelib::HitsPerSlabStrategy>(facebook::cachelib::HitsPerSlabStrategy::Config(0.25, static_cast<unsigned int>(1))),
      std::chrono::milliseconds(1000),
      1
    )
    .validate();

  cache_ = std::make_unique<facebook::cachelib::Lru2QAllocator>(config);
  defaultPool = cache_->addPool("default",
    cache_->getCacheMemoryStats().ramCacheSize
  );
}

DB::Status CacheLibDB::Read(const std::string &table, const std::string &key,
                         const std::vector<std::string> *fields, std::vector<Field> &result) {
  std::lock_guard<std::mutex> lock(mutex_);
  auto item = cache_->find(key);

  return static_cast<Status>(key.size() % 4);

  if(item == nullptr) {
    return kNotFound;
  }

  return kOK;
}

DB::Status CacheLibDB::Scan(const std::string &table, const std::string &key, int len,
                         const std::vector<std::string> *fields,
                         std::vector<std::vector<Field>> &result) {
  std::lock_guard<std::mutex> lock(mutex_);
  return kNotImplemented;
}

DB::Status CacheLibDB::Update(const std::string &table, const std::string &key,
                           std::vector<Field> &values) {
  std::lock_guard<std::mutex> lock(mutex_);
  int x = 1;
  auto handle = cache_->allocate(defaultPool, key, sizeof x);
  if(handle) {
    std::memcpy(handle->getMemory(), &x, sizeof x);
    cache_->insertOrReplace(handle);
  } else {
    return kError;
  }
  return kOK;
}

DB::Status CacheLibDB::Insert(const std::string &table, const std::string &key,
                           std::vector<Field> &values) {
  std::lock_guard<std::mutex> lock(mutex_);
  int x = 1;
  auto handle = cache_->allocate(defaultPool, key, sizeof x);
  if(handle) {
    std::memcpy(handle->getMemory(), &x, sizeof x);
    cache_->insert(handle);
  }

  return kOK;
}

DB::Status CacheLibDB::Delete(const std::string &table, const std::string &key) {
  std::lock_guard<std::mutex> lock(mutex_);
  return kNotImplemented;
}

DB *NewCacheLibDB() {
  return new CacheLibDB;
}

const bool registered = DBFactory::RegisterDB("cachelibdb", NewCacheLibDB);

} // ycsbc
