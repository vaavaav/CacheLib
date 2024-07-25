#pragma once
#include <cachelib/holpaca/control-plane/algorithms/ControlAlgorithm.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>

namespace facebook {
namespace cachelib {
namespace holpaca {
class HitRatioMaximization : public ControlAlgorithm {
  std::shared_ptr<ProxyManager> m_proxyManager;
  std::unordered_map<std::string, double> m_hitRatioQoS;
  std::atomic_bool m_continue{true};
  std::chrono::milliseconds m_periodicity;
  std::set<std::string> m_active{};

  const uint32_t m_kMRCMinLength{3};
  double m_delta{0.05};

 public:
  struct Metadata;

 private:
  Metadata collect(
      std::unordered_map<std::string, std::shared_ptr<Cache>> caches);
  std::unordered_map<int32_t, uint64_t> compute(Metadata& metadata);

 public:
  HitRatioMaximization(
      std::shared_ptr<ProxyManager>& proxyManager,
      std::chrono::milliseconds periodicity,
      double delta,
      std::unordered_map<std::string, double> hitRatioQoS = {});
  void operator()() override final;
};
} // namespace holpaca
} // namespace cachelib
} // namespace facebook