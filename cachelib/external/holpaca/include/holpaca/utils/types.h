#pragma once
#include <cstddef>
#include <chrono>

namespace holpaca {
    using namespace std::chrono_literals;
    using time_t = std::chrono::seconds;
    using id_t = size_t; 
    using hits_t = size_t;
    using lookups_t = size_t;
    using timestamp_t = std::time_t;
    using cache_size_t = size_t;
    using hit_rate_t = int;
}