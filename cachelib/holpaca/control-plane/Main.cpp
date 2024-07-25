#include <iostream>
#include <unordered_map>

#include "cachelib/holpaca/control-plane/Controller.h"
#include "cachelib/holpaca/control-plane/algorithms/ControlAlgorithm.h"
#include "cachelib/holpaca/control-plane/algorithms/HitRatioMaximization.h"

int main(int argc, char** argv) {
  if (argc < 2) {
    std::cerr << "Usage: " << argv[0] << " <address> [<control-algorithm>]"
              << std::endl;
    return 1;
  }
  facebook::cachelib::holpaca::ControllerConfig config;
  config.setAddress(argv[1]).validate();

  for (int i = 2; i < argc; i++) {
    if (std::string(argv[i]) == "HitRatioMaximization") {
      //      config.addAlgorithm<facebook::cachelib::holpaca::HitRatioMaximization>();

    } else {
      std::cerr << "Unknown control algorithm: " << argv[i] << std::endl;
      return 1;
    }
  }

  facebook::cachelib::holpaca::Controller controller(config);
  // If something gets in stdin then we will exit
  std::string ignore;
  std::getline(std::cin, ignore);
  return 0;
}