#pragma once
#include <holpaca/Status.h>

namespace holpaca {
class ControlAPI {
 public:
  virtual void resize(size_t newSize) = 0;
  virtual Status getStatus() = 0;
};
}