#include <holpaca/control-plane/Controller.h>

#include <iostream>
#include <thread>

int main(int argc, char** argv) {
  holpaca::ControllerConfig config;
  config.setLogger("/tmp/controller.log")
      .setRegistrationAddress("localhost:50051");
  if (!config.isValid()) {
    std::cerr << "Invalid controller" << std::endl;
    return 1;
  }
  holpaca::Controller controller(config);

  while (true) {
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }

  return 0;
}