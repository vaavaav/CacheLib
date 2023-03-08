#pragma once
#include <chrono>
#include <holpaca/utils/types.h>

namespace holpaca::control_plane::config {
    char constexpr controller_registry_socket_address[]{"tcp://127.0.0.1:5559"};
    char constexpr controller_log_file[]{"/tmp/controller.txt"};
    char constexpr stage_proxy_log_file[]{"/tmp/stage_proxy.txt"};
    auto constexpr control_algorithm_periodicity { 1s };
}

namespace holpaca::data_plane::config {
    char constexpr stage_address[] = {"ipc://stage_default"};
    char constexpr stage_log_file[]{"/tmp/stage.txt"};
    auto constexpr stage_timeout { 3s };
}
