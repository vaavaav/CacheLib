#pragma once
#include <chrono>
#include <string>
#include <holpaca/utils/types.h>

using namespace std::chrono_literals;

namespace holpaca::control_plane::config {
    char constexpr controller_registry_socket_address[]{"tcp://127.0.0.1:5559"};
    char constexpr controller_log_file[] { "/tmp/controller.log" };
    auto constexpr control_algorithm_periodicity { 1s };
}

namespace holpaca::data_plane::config {
    char constexpr stage_log_file[] { "/tmp/stage.log" };
    char constexpr stage_address[] = {"ipc://stage_default"};
    auto constexpr stage_timeout { 3s };
}
