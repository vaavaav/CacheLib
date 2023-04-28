#ifndef YCSB_C_CACHELIB_DB_H_
#define YCSB_C_CACHELIB_DB_H_

#include "core/db.h"
#include "core/properties.h"

#include <iostream>
#include <string>
#include <mutex>

#include <cachelib/allocator/CacheAllocator.h>

namespace ycsbc {


class CacheLibDB : public DB {
 public:
  void Init();

  Status Read(const std::string &table, const std::string &key,
              const std::vector<std::string> *fields, std::vector<Field> &result);

  Status Scan(const std::string &table, const std::string &key, int len,
              const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result);

  Status Update(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Insert(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Delete(const std::string &table, const std::string &key);

 private:
  static std::mutex mutex_;
  static std::unique_ptr<facebook::cachelib::Lru2QAllocator> cache_;
  static facebook::cachelib::PoolId defaultPool;
};

DB *NewCacheLibDB();

} // ycsbc

#endif // YCSB_C_CACHELIBDB_DB_H_

