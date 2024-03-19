#include <holpaca/control-plane/ProxyManager.h>

#include <mutex>

namespace holpaca {
bool ProxyManager::add(std::string& address) {
  StageProxy newProxy;
  newProxy.connect(address);
  if (!newProxy.isValid() || !newProxy.isConnected()) {
    return false;
  }
  std::lock_guard<std::shared_timed_mutex> lock(m_mutex);
  m_proxies[address] = std::make_shared<StageProxy>(std::move(newProxy));
  return true;
}

void ProxyManager::remove(std::string& address) {
  std::lock_guard<std::shared_timed_mutex> lock(m_mutex);
  m_proxies.erase(address);
}

ProxyManager::iterator ProxyManager::begin() {
  std::shared_lock<std::shared_timed_mutex> lock(m_mutex);
  return m_proxies.begin();
}

ProxyManager::iterator ProxyManager::end() {
  std::shared_lock<std::shared_timed_mutex> lock(m_mutex);
  return m_proxies.end();
}

} // namespace holpaca
