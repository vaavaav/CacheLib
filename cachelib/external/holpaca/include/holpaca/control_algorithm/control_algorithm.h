#pragma once

namespace holpaca::control_algorithm {
class ControlAlgorithm {
 public:
  ControlAlgorithm() = default;
  virtual void run() = 0;
};
} // namespace holpaca::control_algorithm