#pragma once
#include <chrono>
#include <string>

using namespace std::chrono_literals;

namespace holpaca {
    using Id = uint32_t;
}

namespace holpaca::control_plane::config {
    char constexpr controller_log_file[] { "/tmp/controller.log" };
    auto constexpr control_algorithm_periodicity { 1s };
}

namespace holpaca::data_plane::config {
    char constexpr stage_log_file[] { "/tmp/stage.log" };
    char constexpr stage_address[] = {"localhost:7001"};
    auto constexpr stage_timeout { 3s };
}
