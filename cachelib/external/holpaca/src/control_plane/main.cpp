#include <holpaca/control_plane/controller.h>
#include <iostream>
#include <thread>

int main() {
    holpaca::control_plane::Controller c(1000ms);

    while(true){
        std::this_thread::sleep_for(1s);
    }

    return 0;
}