#pragma once
#include <holpaca/data-plane/StageConfig.h>

#include <memory>
#include <thread>

namespace holpaca {
class Stage {
  class Impl;
  std::unique_ptr<Impl> m_impl;

 public:
  Stage(StageConfig* config);
};
} // namespace holpaca
