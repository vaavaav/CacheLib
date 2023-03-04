#pragma once
#include <holpaca/utils/types.h>
#include <chrono>

namespace holpaca::config {
    namespace control_plane {
        char constexpr controller_registry_socket_address[]{"tcp://127.0.0.1:5559"};
        char constexpr controller_log_file[]{"/tmp/controller.txt"};
        char constexpr stage_proxy_log_file[]{"/tmp/stage_proxy.txt"};
    }
    namespace data_plane {
        char constexpr stage_socket_address[] = {"ipc://stage_default"};
        char constexpr stage_log_file[]{"/tmp/stage.txt"};
        auto constexpr stage_timeout { 3s };
    }
}
