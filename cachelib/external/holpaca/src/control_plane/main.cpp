#include <holpaca/control_plane/controller.h>

#include <iostream>
#include <thread>

int main(int argc, char** argv) {
  // if no arguments are provided, then function simply returns
  if (argc < 2) {
    return 0;
  }
  // first argument is the periodicity of the control algorithm
  std::chrono::milliseconds periodicity{strtol(argv[1], NULL, 10)};
  // second argument is the name of the control algorithm

  holpaca::control_plane::Controller c(periodicity, "/tmp/controller.log",
                                       &argv[2]);

  while (true) {
    std::this_thread::sleep_for(1s);
  }

  return 0;
}