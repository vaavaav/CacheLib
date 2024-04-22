#ifndef YCSB_C_CACHELIB_DB_H_
#define YCSB_C_CACHELIB_DB_H_

#include <cachelib/allocator/CacheAllocator.h>

#include <iostream>
#include <mutex>
#include <string>
#include <unordered_map>

#include "core/db.h"
#include "core/properties.h"
#include "rocksdb.h"

namespace ycsbc {

class CacheLib : public DB {
  using CacheLibAllocator = facebook::cachelib::Lru2QAllocator;

 public:
  void Init();

  Status Read(const std::string& table,
              const std::string& key,
              const std::vector<std::string>* fields,
              std::vector<Field>& result);

  Status Scan(const std::string& table,
              const std::string& key,
              long len,
              const std::vector<std::string>* fields,
              std::vector<std::vector<Field>>& result);

  Status Update(const std::string& table,
                const std::string& key,
                std::vector<Field>& values);

  Status Insert(const std::string& table,
                const std::string& key,
                std::vector<Field>& values);

  Status Delete(const std::string& table, const std::string& key);

  static void SerializeRow(const std::vector<Field>& values, std::string& data);
  static void DeserializeRowFilter(std::vector<Field>& values,
                                   const char* p,
                                   const char* lim,
                                   const std::vector<std::string>& fields);
  static void DeserializeRowFilter(std::vector<Field>& values,
                                   const std::string& data,
                                   const std::vector<std::string>& fields);
  static void DeserializeRow(std::vector<Field>& values,
                             const char* p,
                             const char* lim);
  static void DeserializeRow(std::vector<Field>& values,
                             const std::string& data);

  // ROCKSDB

 private:
  static RocksDB rocksdb_;
  static std::mutex mutex_;
  static int ref_cnt_;
  static std::unique_ptr<CacheLibAllocator> cache_;
  static std::vector<facebook::cachelib::PoolId> pools_;
  thread_local static int threadId_;
};

DB* NewCacheLib();

} // namespace ycsbc

#endif // YCSB_C_CACHELIBDB_DB_H_
