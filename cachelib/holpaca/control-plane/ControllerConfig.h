#pragma once
#include <memory>
#include <string>
#include <unordered_map>

#include "cachelib/holpaca/control-plane/algorithms/ControlAlgorithm.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class ControllerConfig {
  std::string m_address;
  std::unordered_map<std::string, std::shared_ptr<ControlAlgorithm>>
      m_algorithms;

 public:
  ControllerConfig& setAddress(std::string address) {
    m_address = address;
    return *this;
  }

  template <typename T, typename... Args>
  ControllerConfig& addAlgorithm(Args... args) {
    m_algorithms[typename T::name()] = std::make_shared<T>(args...);

    return *this;
  }

  const ControllerConfig& validate() const {
    if (m_address.empty()) {
      throw std::invalid_argument("Address for this instance must be set");
    }
    return *this;
  }

  friend class Controller;
};

} // namespace holpaca
} // namespace cachelib
} // namespace facebook