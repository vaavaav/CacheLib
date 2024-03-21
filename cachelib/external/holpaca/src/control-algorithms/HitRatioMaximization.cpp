#include <holpaca/control-algorithms/HitRatioMaximization.h>

#include <map>
#include <thread>

struct CollectInformation {
  std::unordered_map<std::string, size_t> partitioning;
  std::map<uint64_t, double> mrcs;
};

struct ComputeInformation {
  std::unordered_map<std::string, size_t> newPartitioning;
};

CollectInformation collect(
    std::unordered_map<std::string, std::shared_ptr<holpaca::Cache>>& caches) {
  CollectInformation collectInformation;
  for (const auto& [address, cache] : caches) {
    auto status = cache->getStatus();
    collectInformation.partitioning[address] = status.maxSize;
  }
  return collectInformation;
}

ComputeInformation compute(const CollectInformation& collectInformation) {
  ComputeInformation computeInformation;
  // todo
  return computeInformation;
}

namespace holpaca {
HitRatioMaximization::HitRatioMaximization(
    std::shared_ptr<ProxyManager>& proxyManager)
    : ControlAlgorithm(proxyManager) {}

void HitRatioMaximization::operator()() {
  while (m_continue) {
    auto caches = m_proxyManager->getCaches();
    auto collectInformation = collect(caches);
    auto computeInformation = compute(collectInformation);

    for (const auto& [address, partition] :
         computeInformation.newPartitioning) {
      caches[address]->resize(partition);
    }

    std::this_thread::sleep_for(m_periodicity);
  }
}

} // namespace holpaca
