#pragma once

#include <string>

#include "cachelib/holpaca/control-plane/ProxyManager.h"

namespace facebook {
namespace cachelib {
namespace holpaca {
class ControlAlgorithm {
 protected:
  std::shared_ptr<ProxyManager> m_proxyManager;

 public:
  ControlAlgorithm(std::shared_ptr<ProxyManager> proxyManager) {
    m_proxyManager = proxyManager;
  }
  virtual void operator()() = 0;
  virtual void stop() = 0;
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook