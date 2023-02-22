#pragma once
#include <cstddef>
#include <chrono>

namespace alpaca::data_plane_stage::substage::monitor {

    using hit_rate_t = int;
    using cache_size_t = size_t;
    using timestamp_t = std::time_t;

    class Snapshot {
        public: 
            Snapshot(timestamp_t& const timestamp, hit_rate_t& const hit_rate, cache_size_t& const cache_size);
            timestamp_t const timestamp;
            cache_size_t const cache_size;
            hit_rate_t const hit_rate;
    };

    struct message {
        id_t id;
        Snapshot snapshot[];
    };
}