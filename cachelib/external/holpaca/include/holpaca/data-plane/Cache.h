#pragma once
#include <cstdint>
#include <string>

namespace holpaca {
class Cache {
 public:
  virtual void put(std::string& key, void* value, size_t valueSize) = 0;
  virtual void* get(uint64_t key) = 0;
  virtual void remove(uint64_t key) = 0;
};
}