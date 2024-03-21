#pragma once
#include <holpaca/control-plane/ControlAlgorithm.h>

#include <atomic>
#include <unordered_map>

namespace holpaca {
class HitRatioMaximization : public ControlAlgorithm {
  std::unordered_map<std::string, double> m_hitRatioQoS;
  std::atomic_bool m_continue{true};

 public:
  HitRatioMaximization(std::shared_ptr<ProxyManager>& proxyManager);
  void operator()() override final;
  HitRatioMaximization& fromFile(std::string& configFile);
  HitRatioMaximization& setPeriodicity(std::chrono::milliseconds periodicity);
  HitRatioMaximization& setHitRatioQoS(
      std::unordered_map<std::string, double>& hitRatioQoS);

  bool isValid() const override final;
};
} // namespace holpaca