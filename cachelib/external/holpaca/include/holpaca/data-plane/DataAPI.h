#pragma once
#include <holpaca/data-plane/ControlAPI.h>

#include <cstdint>
#include <string>

namespace holpaca {
class DataAPI {
 public:
  virtual void put(std::string& key, void* value, size_t valueSize) = 0;
  virtual void* get(uint64_t key) = 0;
  virtual void remove(uint64_t key) = 0;
};
} // namespace holpaca