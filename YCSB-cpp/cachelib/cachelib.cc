#include "cachelib.h"

#include "core/db_factory.h"
using std::cout;
using std::endl;
#include <sstream>
#include <unordered_map>
#include <rocksdb/cache.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/merge_operator.h>
#include <rocksdb/status.h>
#include <rocksdb/utilities/options_util.h>
#include <rocksdb/write_batch.h>

#include "cachelib/allocator/CacheAllocator.h"
#include "cachelib/allocator/HitsPerSlabStrategy.h"
#include "cachelib/allocator/PoolOptimizeStrategy.h"
#include "cachelib/allocator/MarginalHitsOptimizeStrategy.h"

namespace {
const std::string PROP_CACHE_SIZE = "cachelib.cachesize";
const std::string PROP_CACHE_SIZE_DEFAULT =
    std::to_string(1 * 1024 * 1024 * 1024);

const std::string PROP_CACHE_ALLOCATOR = "cachelib.cache_allocator";
const std::string PROP_CACHE_ALLOCATOR_DEFAULT = "lru";

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

const std::string PROP_HOLPACA = "cachelib.holpaca";
const std::string PROP_HOLPACA_DEFAULT = "off";

const std::string PROP_HOLPACA_PERIODICITY = "cachelib.holpaca_periodicity";
const std::string PROP_HOLPACA_PERIODICITY_DEFAULT = "1000";

// ROCKSDB
const std::string PROP_NAME = "rocksdb.dbname";
const std::string PROP_NAME_DEFAULT = "";

const std::string PROP_FORMAT = "rocksdb.format";
const std::string PROP_FORMAT_DEFAULT = "single";

const std::string PROP_MERGEUPDATE = "rocksdb.mergeupdate";
const std::string PROP_MERGEUPDATE_DEFAULT = "false";

const std::string PROP_DESTROY = "rocksdb.destroy";
const std::string PROP_DESTROY_DEFAULT = "false";

const std::string PROP_COMPRESSION = "rocksdb.compression";
const std::string PROP_COMPRESSION_DEFAULT = "no";

const std::string PROP_MAX_BG_JOBS = "rocksdb.max_background_jobs";
const std::string PROP_MAX_BG_JOBS_DEFAULT = "0";

const std::string PROP_MAX_BG_FLUSHES = "rocksdb.max_background_flushes";
const std::string PROP_MAX_BG_FLUSHES_DEFAULT = "1";

const std::string PROP_MAX_BG_COMPACTIONS = "rocksdb.max_background_compactions";
const std::string PROP_MAX_BG_COMPACTIONS_DEFAULT = "3";

const std::string PROP_TARGET_FILE_SIZE_BASE = "rocksdb.target_file_size_base";
const std::string PROP_TARGET_FILE_SIZE_BASE_DEFAULT = "0";

const std::string PROP_TARGET_FILE_SIZE_MULT =
    "rocksdb.target_file_size_multiplier";
const std::string PROP_TARGET_FILE_SIZE_MULT_DEFAULT = "0";

const std::string PROP_MAX_BYTES_FOR_LEVEL_BASE =
    "rocksdb.max_bytes_for_level_base";
const std::string PROP_MAX_BYTES_FOR_LEVEL_BASE_DEFAULT = "0";

const std::string PROP_WRITE_BUFFER_SIZE = "rocksdb.write_buffer_size";
const std::string PROP_WRITE_BUFFER_SIZE_DEFAULT = "0";

const std::string PROP_MAX_WRITE_BUFFER = "rocksdb.max_write_buffer_number";
const std::string PROP_MAX_WRITE_BUFFER_DEFAULT = "0";

const std::string PROP_COMPACTION_PRI = "rocksdb.compaction_pri";
const std::string PROP_COMPACTION_PRI_DEFAULT = "-1";

const std::string PROP_MAX_OPEN_FILES = "rocksdb.max_open_files";
const std::string PROP_MAX_OPEN_FILES_DEFAULT = "-1";

const std::string PROP_L0_COMPACTION_TRIGGER =
    "rocksdb.level0_file_num_compaction_trigger";
const std::string PROP_L0_COMPACTION_TRIGGER_DEFAULT = "0";

const std::string PROP_L0_SLOWDOWN_TRIGGER =
    "rocksdb.level0_slowdown_writes_trigger";
const std::string PROP_L0_SLOWDOWN_TRIGGER_DEFAULT = "0";

const std::string PROP_L0_STOP_TRIGGER = "rocksdb.level0_stop_writes_trigger";
const std::string PROP_L0_STOP_TRIGGER_DEFAULT = "0";

const std::string PROP_USE_DIRECT_WRITE =
    "rocksdb.use_direct_io_for_flush_compaction";
const std::string PROP_USE_DIRECT_WRITE_DEFAULT = "false";

const std::string PROP_USE_DIRECT_READ = "rocksdb.use_direct_reads";
const std::string PROP_USE_DIRECT_READ_DEFAULT = "false";

const std::string PROP_USE_MMAP_WRITE = "rocksdb.allow_mmap_writes";
const std::string PROP_USE_MMAP_WRITE_DEFAULT = "false";

const std::string PROP_USE_MMAP_READ = "rocksdb.allow_mmap_reads";
const std::string PROP_USE_MMAP_READ_DEFAULT = "false";

const std::string PROP_BLOOM_BITS = "rocksdb.bloom_bits";
const std::string PROP_BLOOM_BITS_DEFAULT = "0";

const std::string PROP_INCREASE_PARALLELISM = "rocksdb.increase_parallelism";
const std::string PROP_INCREASE_PARALLELISM_DEFAULT = "false";

const std::string PROP_OPTIMIZE_LEVELCOMP =
    "rocksdb.optimize_level_style_compaction";
const std::string PROP_OPTIMIZE_LEVELCOMP_DEFAULT = "false";

const std::string PROP_OPTIONS_FILE = "rocksdb.optionsfile";
const std::string PROP_OPTIONS_FILE_DEFAULT = "";
} // namespace

namespace ycsbc {

std::mutex CacheLib::mutex_;
std::unique_ptr<CacheLibAllocator> CacheLib::cache_ = nullptr;
rocksdb::DB* CacheLib::db_ = nullptr;
int CacheLib::ref_cnt_ = 0;
std::unordered_map<std::thread::id, facebook::cachelib::PoolId>
    CacheLib::pools_;

void CacheLib::Init() {
  RDInit();
  std::lock_guard<std::mutex> lock(mutex_);
  if (cache_ == nullptr) {
    CacheLibAllocator::Config config;
    config
        .setCacheSize(std::stoi(
            props_->GetProperty(PROP_CACHE_SIZE, PROP_CACHE_SIZE_DEFAULT)))
        .setCacheName("YCSBenchmark")
        .setAccessConfig({25, 10});
    if (props_->GetProperty(PROP_TAIL_HITS_TRACKING,
                            PROP_TAIL_HITS_TRACKING_DEFAULT) == "on") {
      config.enableTailHitsTracking();
      if (props_->GetProperty(PROP_POOL_OPTIMIZER,
                              PROP_POOL_OPTIMIZER_DEFAULT) == "on") {
        config.enablePoolOptimizer(
            std::make_shared<facebook::cachelib::MarginalHitsOptimizeStrategy>(),
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
    config.validate();

    cache_ = std::make_unique<CacheLibAllocator>(config);
  }
  std::ostringstream threadId;
  threadId << std::this_thread::get_id();
  pools_[std::this_thread::get_id()] =
      cache_->addPool(threadId.str(),
                      cache_->getCacheMemoryStats().ramCacheSize /
                          std::stof(props_->GetProperty("threadcount", "1")));
}

DB::Status CacheLib::Read(const std::string& table,
                          const std::string& key,
                          const std::vector<std::string>* fields,
                          std::vector<Field>& result) {
  auto handle = cache_->find(key);

  if (handle == nullptr) {
    if(RDRead(table, key, fields, result) != kNotFound) {
      std::string data = result.front().value;
      auto handle =
        cache_->allocate(pools_[std::this_thread::get_id()], key, data.size());

      if (handle) {
        std::memcpy(handle->getMemory(), data.data(), data.size());
        cache_->insertOrReplace(handle);
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
                          int len,
                          const std::vector<std::string>* fields,
                          std::vector<std::vector<Field>>& result) {
  // seek key
  auto itr = cache_->begin();
  for (; itr != cache_->end(); ++itr) {
    if (key.compare(itr->getKey()) == 0) {
      break;
    }
  }

  for (int i = 0; i < len; ++i) {
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
  auto item = cache_->find(key);
  RDUpdate(table, key, values);

  std::string data = values.front().value;

  auto handle =
      cache_->allocate(pools_[std::this_thread::get_id()], key, data.size());

  if (handle) {
    std::memcpy(handle->getMemory(), data.data(), data.size());
    cache_->insertOrReplace(handle);
  } else {
    return kError;
  }

  return item == nullptr ? kNotFound : kOK;
}

DB::Status CacheLib::Insert(const std::string& table,
                            const std::string& key,
                            std::vector<Field>& values) {
  RDInsert(table, key, values);

  return kOK;
}

DB::Status CacheLib::Delete(const std::string& table, const std::string& key) {
  return cache_->remove(key) == CacheLibAllocator::RemoveRes::kSuccess
             ? kOK
             : kNotFound;
}

void CacheLib::RDInit() {
  const std::lock_guard<std::mutex> lock(mutex_);

  ref_cnt_++;
  if (db_) {
    return;
  }

  const std::string& db_path = props_->GetProperty(PROP_NAME, PROP_NAME_DEFAULT);
  if (db_path == "") {
    throw utils::Exception("RocksDB db path is missing");
  }

  rocksdb::Options opt;
  opt.create_if_missing = true;
  std::vector<rocksdb::ColumnFamilyDescriptor> cf_descs;
  std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
  RDGetOptions(*props_, &opt, &cf_descs);

  rocksdb::Status s;
  if (props_->GetProperty(PROP_DESTROY, PROP_DESTROY_DEFAULT) == "true") {
    s = rocksdb::DestroyDB(db_path, opt);
    if (!s.ok()) {
      throw utils::Exception(std::string("RocksDB DestroyDB: ") + s.ToString());
    }
  }
  if (cf_descs.empty()) {
    s = rocksdb::DB::Open(opt, db_path, &db_);
  } else {
    s = rocksdb::DB::Open(opt, db_path, cf_descs, &cf_handles, &db_);
  }
  if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Open: ") + s.ToString());
  }
}

void CacheLib::RDCleanup() {
  const std::lock_guard<std::mutex> lock(mutex_);
  if (--ref_cnt_) {
    return;
  }
  delete db_;
}

void CacheLib::RDGetOptions(
    const utils::Properties& props,
    rocksdb::Options* opt,
    std::vector<rocksdb::ColumnFamilyDescriptor>* cf_descs) {
  rocksdb::Env* env = rocksdb::Env::Default();
  ;
  const std::string options_file =
      props_->GetProperty(PROP_OPTIONS_FILE, PROP_OPTIONS_FILE_DEFAULT);
  if (options_file != "") {
    rocksdb::ConfigOptions config_options;
    config_options.ignore_unknown_options = false;
    config_options.input_strings_escaped = true;
    config_options.env = env;
    rocksdb::Status s = rocksdb::LoadOptionsFromFile(
        config_options, options_file, opt, cf_descs);
    if (!s.ok()) {
      throw utils::Exception(std::string("RocksDB LoadOptionsFromFile: ") +
                             s.ToString());
    }
  } else {
    const std::string compression_type =
        props_->GetProperty(PROP_COMPRESSION, PROP_COMPRESSION_DEFAULT);
    if (compression_type == "no") {
      opt->compression = rocksdb::kNoCompression;
    } else if (compression_type == "snappy") {
      opt->compression = rocksdb::kSnappyCompression;
    } else if (compression_type == "zlib") {
      opt->compression = rocksdb::kZlibCompression;
    } else if (compression_type == "bzip2") {
      opt->compression = rocksdb::kBZip2Compression;
    } else if (compression_type == "lz4") {
      opt->compression = rocksdb::kLZ4Compression;
    } else if (compression_type == "lz4hc") {
      opt->compression = rocksdb::kLZ4HCCompression;
    } else if (compression_type == "xpress") {
      opt->compression = rocksdb::kXpressCompression;
    } else if (compression_type == "zstd") {
      opt->compression = rocksdb::kZSTD;
    } else {
      throw utils::Exception("Unknown compression type");
    }

    int val = std::stoi(
        props_->GetProperty(PROP_MAX_BG_FLUSHES, PROP_MAX_BG_FLUSHES_DEFAULT));
    if (val != 0) {
      opt->max_background_flushes = val;
    }
    val = std::stoi(
        props_->GetProperty(PROP_MAX_BG_COMPACTIONS, PROP_MAX_BG_COMPACTIONS_DEFAULT));
    if (val != 0) {
      opt->max_background_compactions = val;
    }
    val = std::stoi(
        props_->GetProperty(PROP_MAX_BG_JOBS, PROP_MAX_BG_JOBS_DEFAULT));
    if (val != 0) {
      opt->max_background_jobs = val;
    }
    val = std::stoi(props_->GetProperty(PROP_TARGET_FILE_SIZE_BASE,
                                      PROP_TARGET_FILE_SIZE_BASE_DEFAULT));
    if (val != 0) {
      opt->target_file_size_base = val;
    }
    val = std::stoi(props_->GetProperty(PROP_TARGET_FILE_SIZE_MULT,
                                      PROP_TARGET_FILE_SIZE_MULT_DEFAULT));
    if (val != 0) {
      opt->target_file_size_multiplier = val;
    }
    val = std::stoi(props_->GetProperty(PROP_MAX_BYTES_FOR_LEVEL_BASE,
                                      PROP_MAX_BYTES_FOR_LEVEL_BASE_DEFAULT));
    if (val != 0) {
      opt->max_bytes_for_level_base = val;
    }
    val = std::stoi(props_->GetProperty(PROP_WRITE_BUFFER_SIZE,
                                      PROP_WRITE_BUFFER_SIZE_DEFAULT));
    if (val != 0) {
      opt->write_buffer_size = val;
    }
    val = std::stoi(props_->GetProperty(PROP_MAX_WRITE_BUFFER,
                                      PROP_MAX_WRITE_BUFFER_DEFAULT));
    if (val != 0) {
      opt->max_write_buffer_number = val;
    }
    val = std::stoi(
        props_->GetProperty(PROP_COMPACTION_PRI, PROP_COMPACTION_PRI_DEFAULT));
    if (val != -1) {
      opt->compaction_pri = static_cast<rocksdb::CompactionPri>(val);
    }
    val = std::stoi(
        props_->GetProperty(PROP_MAX_OPEN_FILES, PROP_MAX_OPEN_FILES_DEFAULT));
    if (val != 0) {
      opt->max_open_files = val;
    }

    val = std::stoi(props_->GetProperty(PROP_L0_COMPACTION_TRIGGER,
                                      PROP_L0_COMPACTION_TRIGGER_DEFAULT));
    if (val != 0) {
      opt->level0_file_num_compaction_trigger = val;
    }
    val = std::stoi(props_->GetProperty(PROP_L0_SLOWDOWN_TRIGGER,
                                      PROP_L0_SLOWDOWN_TRIGGER_DEFAULT));
    if (val != 0) {
      opt->level0_slowdown_writes_trigger = val;
    }
    val = std::stoi(
        props_->GetProperty(PROP_L0_STOP_TRIGGER, PROP_L0_STOP_TRIGGER_DEFAULT));
    if (val != 0) {
      opt->level0_stop_writes_trigger = val;
    }

    if (props_->GetProperty(PROP_USE_DIRECT_WRITE,
                          PROP_USE_DIRECT_WRITE_DEFAULT) == "true") {
      opt->use_direct_io_for_flush_and_compaction = true;
    }
    if (props_->GetProperty(PROP_USE_DIRECT_READ, PROP_USE_DIRECT_READ_DEFAULT) ==
        "true") {
      opt->use_direct_reads = true;
    }
    if (props_->GetProperty(PROP_USE_MMAP_WRITE, PROP_USE_MMAP_WRITE_DEFAULT) ==
        "true") {
      opt->allow_mmap_writes = true;
    }
    if (props_->GetProperty(PROP_USE_MMAP_READ, PROP_USE_MMAP_READ_DEFAULT) ==
        "true") {
      opt->allow_mmap_reads = true;
    }

    rocksdb::BlockBasedTableOptions table_options;
    size_t cache_size = 0;
    int bloom_bits =
        std::stoul(props_->GetProperty(PROP_BLOOM_BITS, PROP_BLOOM_BITS_DEFAULT));
    if (bloom_bits > 0) {
      table_options.filter_policy.reset(
          rocksdb::NewBloomFilterPolicy(bloom_bits));
    }
    opt->table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));

    if (props_->GetProperty(PROP_INCREASE_PARALLELISM,
                          PROP_INCREASE_PARALLELISM_DEFAULT) == "true") {
      opt->IncreaseParallelism();
    }
    if (props_->GetProperty(PROP_OPTIMIZE_LEVELCOMP,
                          PROP_OPTIMIZE_LEVELCOMP_DEFAULT) == "true") {
      opt->OptimizeLevelStyleCompaction();
    }
  }
}

void CacheLib::SerializeRow(const std::vector<Field>& values,
                            std::string& data) {
  for (const Field& field : values) {
    uint32_t len = field.name.size();
    data.append(reinterpret_cast<char*>(&len), sizeof(uint32_t));
    data.append(field.name.data(), field.name.size());
    len = field.value.size();
    data.append(reinterpret_cast<char*>(&len), sizeof(uint32_t));
    data.append(field.value.data(), field.value.size());
  }
}

void CacheLib::DeserializeRowFilter(std::vector<Field>& values,
                                    const char* p,
                                    const char* lim,
                                    const std::vector<std::string>& fields) {
  std::vector<std::string>::const_iterator filter_iter = fields.begin();
  while (p != lim && filter_iter != fields.end()) {
    assert(p < lim);
    uint32_t len = *reinterpret_cast<const uint32_t*>(p);
    p += sizeof(uint32_t);
    std::string field(p, static_cast<const size_t>(len));
    p += len;
    len = *reinterpret_cast<const uint32_t*>(p);
    p += sizeof(uint32_t);
    std::string value(p, static_cast<const size_t>(len));
    p += len;
    if (*filter_iter == field) {
      values.push_back({field, value});
      filter_iter++;
    }
  }
  assert(values.size() == fields.size());
}

void CacheLib::DeserializeRowFilter(std::vector<Field>& values,
                                    const std::string& data,
                                    const std::vector<std::string>& fields) {
  const char* p = data.data();
  const char* lim = p + data.size();
  DeserializeRowFilter(values, p, lim, fields);
}

void CacheLib::DeserializeRow(std::vector<Field>& values,
                              const char* p,
                              const char* lim) {
  while (p != lim) {
    assert(p < lim);
    uint32_t len = *reinterpret_cast<const uint32_t*>(p);
    p += sizeof(uint32_t);
    std::string field(p, static_cast<const size_t>(len));
    p += len;
    len = *reinterpret_cast<const uint32_t*>(p);
    p += sizeof(uint32_t);
    std::string value(p, static_cast<const size_t>(len));
    p += len;
    values.push_back({field, value});
  }
}

void CacheLib::DeserializeRow(std::vector<Field>& values,
                              const std::string& data) {
  const char* p = data.data();
  const char* lim = p + data.size();
  DeserializeRow(values, p, lim);
}

DB::Status CacheLib::RDRead(const std::string& table,
                            const std::string& key,
                            const std::vector<std::string>* fields,
                            std::vector<Field>& result) {
  std::string data;
  rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &data);
  if (s.IsNotFound()) {
    return kNotFound;
  } else if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Get: ") + s.ToString());
  }
  if (fields != nullptr) {
    DeserializeRowFilter(result, data, *fields);
  } else {
    DeserializeRow(result, data);
  }
  return kOK;
}

DB::Status CacheLib::RDScan(const std::string& table,
                            const std::string& key,
                            int len,
                            const std::vector<std::string>* fields,
                            std::vector<std::vector<Field>>& result) {
  rocksdb::Iterator* db_iter = db_->NewIterator(rocksdb::ReadOptions());
  db_iter->Seek(key);
  for (int i = 0; db_iter->Valid() && i < len; i++) {
    std::string data = db_iter->value().ToString();
    result.push_back(std::vector<Field>());
    std::vector<Field>& values = result.back();
    if (fields != nullptr) {
      DeserializeRowFilter(values, data, *fields);
    } else {
      DeserializeRow(values, data);
    }
    db_iter->Next();
  }
  delete db_iter;
  return kOK;
}

DB::Status CacheLib::RDUpdate(const std::string& table,
                              const std::string& key,
                              std::vector<Field>& values) {
  std::string data;
  rocksdb::Status s = db_->Get(rocksdb::ReadOptions(), key, &data);
  if (s.IsNotFound()) {
    return kNotFound;
  } else if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Get: ") + s.ToString());
  }
  std::vector<Field> current_values;
  DeserializeRow(current_values, data);
  for (Field& new_field : values) {
    bool found MAYBE_UNUSED = false;
    for (Field& cur_field : current_values) {
      if (cur_field.name == new_field.name) {
        found = true;
        cur_field.value = new_field.value;
        break;
      }
    }
    assert(found);
  }
  rocksdb::WriteOptions wopt;
  wopt.disableWAL = true;

  data.clear();
  SerializeRow(current_values, data);
  s = db_->Put(wopt, key, data);
  if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Put: ") + s.ToString());
  }
  return kOK;
}

DB::Status CacheLib::RDInsert(const std::string& table,
                              const std::string& key,
                              std::vector<Field>& values) {
  std::string data;
  SerializeRow(values, data);
  rocksdb::WriteOptions wopt;
  wopt.disableWAL = true;
  rocksdb::Status s = db_->Put(wopt, key, data);
  if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Put: ") + s.ToString());
  }
  return kOK;
}

DB::Status CacheLib::RDDelete(const std::string& table,
                              const std::string& key) {
  rocksdb::WriteOptions wopt;
  rocksdb::Status s = db_->Delete(wopt, key);
  if (!s.ok()) {
    throw utils::Exception(std::string("RocksDB Delete: ") + s.ToString());
  }
  return kOK;
}

DB* NewCacheLib() { return new CacheLib; }

const bool registered = DBFactory::RegisterDB("cachelib", NewCacheLib);

} // namespace ycsbc
