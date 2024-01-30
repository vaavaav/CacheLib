#ifndef YCSB_C_CACHELIB_DB_H_
#define YCSB_C_CACHELIB_DB_H_

#include "core/db.h"
#include "core/properties.h"
#include <rocksdb/db.h>
#include <rocksdb/options.h>

#include <iostream>
#include <string>
#include <mutex>

#include <cachelib/allocator/CacheAllocator.h>
#include <unordered_map>

namespace ycsbc {

using CacheLibAllocator = facebook::cachelib::Lru2QAllocator;

class CacheLib : public DB {
 public:
  void Init();
  void SetThreadId(int id) override;

  Status Read(const std::string &table, const std::string &key,
              const std::vector<std::string> *fields, std::vector<Field> &result);

  Status Scan(const std::string &table, const std::string &key, long len,
              const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result);

  Status Update(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Insert(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status Delete(const std::string &table, const std::string &key);


  Status RDRead(const std::string &table, const std::string &key,
              const std::vector<std::string> *fields, std::vector<Field> &result);

  Status RDScan(const std::string &table, const std::string &key, long len,
              const std::vector<std::string> *fields, std::vector<std::vector<Field>> &result);

  Status RDUpdate(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status RDInsert(const std::string &table, const std::string &key, std::vector<Field> &values);

  Status RDDelete(const std::string &table, const std::string &key);

  void RDInit();
  void RDCleanup();
  
  void RDGetOptions(const utils::Properties &props, rocksdb::Options *opt,
                  std::vector<rocksdb::ColumnFamilyDescriptor> *cf_descs);
  static void SerializeRow(const std::vector<Field> &values, std::string &data);
  static void DeserializeRowFilter(std::vector<Field> &values, const char *p, const char *lim,
                                   const std::vector<std::string> &fields);
  static void DeserializeRowFilter(std::vector<Field> &values, const std::string &data,
                                   const std::vector<std::string> &fields);
  static void DeserializeRow(std::vector<Field> &values, const char *p, const char *lim);
  static void DeserializeRow(std::vector<Field> &values, const std::string &data);

 private:
  static std::mutex mutex_;
  static rocksdb::DB *db_;
  static int ref_cnt_;
  static std::unique_ptr<CacheLibAllocator> cache_;
  static std::vector<facebook::cachelib::PoolId> pools_;
  thread_local static int threadId_;
};

DB *NewCacheLib();

} // ycsbc

#endif // YCSB_C_CACHELIBDB_DB_H_

