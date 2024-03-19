#pragma once
#include <holpaca/control-plane/StageProxy.h>

#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>

namespace holpaca {
class ProxyManager {
  std::unordered_map<std::string, std::shared_ptr<StageProxy>> m_proxies;
  std::shared_timed_mutex m_mutex;

 public:
  ProxyManager() = default;
  bool add(std::string& address);
  void remove(std::string& address);

  using iterator =
      std::unordered_map<std::string, std::shared_ptr<StageProxy>>::iterator;

  iterator begin();
  iterator end();
};
} // namespace holpaca
