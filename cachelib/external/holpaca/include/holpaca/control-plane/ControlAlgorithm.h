#pragma once
#include <string>

namespace holpaca {
class ControlAlgorithm {
 public:
  ControlAlgorithm() = default;
  virtual void operator()() = 0;
  virtual void fromFile(std::string& configFile) = 0;
  virtual std::string& id() const = 0;
  virtual bool isValid() const = 0;
};
} // namespace holpaca