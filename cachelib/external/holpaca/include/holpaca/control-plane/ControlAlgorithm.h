#pragma once
#include <holpaca/control-plane/ProxyManager.h>

#include <string>

namespace holpaca {
class ControlAlgorithm {
 protected:
  std::shared_ptr<ProxyManager> m_proxyManager;

 public:
  ControlAlgorithm(std::shared_ptr<ProxyManager>&);
  virtual void operator()() = 0;
  virtual void stop() = 0;
};
} // namespace holpaca