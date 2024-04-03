#include <holpaca/control-algorithms/HitRatioMaximization.h>
#include <holpaca/ext/Spline.h>

#include <map>
#include <thread>

struct status {
  uint64_t size{0};
  uint64_t freeSize{0};
  tk::spline mrc;
};

using CollectInformation = std::unordered_map<std::string, status>;

struct ComputeInformation {
  std::unordered_map<std::string, size_t> newPartitioning;
};

CollectInformation collect(
    std::unordered_map<std::string, std::shared_ptr<holpaca::Cache>>& caches) {
  CollectInformation collectInformation;
  for (const auto& [address, cache] : caches) {
    auto status = cache->getStatus();
    std::vector<uint64_t> sizes;
    std::vector<double> missRatios;
    sizes.reserve(status.mrc.size());
    missRatios.reserve(status.mrc.size());
    for (auto& [size, missRatio] : status.mrc) {
      sizes.push_back(size);
      missRatios.push_back(missRatio);
    }
    collectInformation[address] = {

        tk::spline(sizes, missRatios, tk::spline::cspline_hermite, true)};
  }
  return collectInformation;
}

ComputeInformation compute(const CollectInformation& collectInformation) {
  ComputeInformation computeInformation;

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
