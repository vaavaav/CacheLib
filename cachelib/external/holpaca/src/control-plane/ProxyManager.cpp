#include <holpaca/Cache.h>
#include <holpaca/control-plane/ProxyManager.h>

#include <mutex>

namespace holpaca
{

  bool ProxyManager::add(std::string &address)
  {
    StageProxy newProxy;
    newProxy.connect(address);
    if (!newProxy.isValid() || !newProxy.isConnected())
    {
      return false;
    }
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);
    m_proxies[address] = std::make_shared<StageProxy>(std::move(newProxy));
    return true;
  }

  void ProxyManager::remove(std::string &address)
  {
    std::lock_guard<std::shared_timed_mutex> lock(m_mutex);
    m_proxies.erase(address);
  }

  std::unordered_map<std::string, std::shared_ptr<ControlInterface>>
  ProxyManager::getCaches()
  {
    std::unordered_map<std::string, std::shared_ptr<ControlInterface>> caches;
    std::shared_lock<std::shared_timed_mutex> lock(m_mutex);
    for (auto &[address, proxy] : m_proxies)
    {
      if (proxy->isConnected())
      {
        caches[address] = std::static_pointer_cast<ControlInterface>(proxy);
      }
    }
    return caches;
  }

  grpc::Status ProxyManager::connect(grpc::ServerContext *context,
                                     const ConnectRequest *request,
                                     ConnectResponse *response)
  {
    std::string address = request->address();
    response->set_success(add(address));
    return grpc::Status::OK;
  }

  grpc::Status ProxyManager::disconnect(grpc::ServerContext *context,
                                        const DisconnectRequest *request,
                                        DisconnectResponse *response)
  {
    std::string address = request->address();
    remove(address);
    return grpc::Status::OK;
  }
} // namespace holpaca
