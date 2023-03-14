#pragma once
#include <holpaca/control_algorithm/control_algorithm.h>
#include <holpaca/data_plane/cache.h>
#include <holpaca/utils/types.h>
#include <cstddef>
#include <memory>
#include <unordered_map>
#include <iostream>
#include <optional>

using namespace holpaca::data_plane;

namespace holpaca::control_algorithm {
    class NaiveControlAlgorithm : public ControlAlgorithm {
        NaiveControlAlgorithm() = delete;
        Cache* cache;

        struct Stats {
            uint32_t hits;
            size_t size; 
        };
        std::unordered_map<id_t, Stats> stats;
        struct Solution {
            Id giver;
            size_t delta;
            Id taker;
        };
        std::optional<Solution> solution;
        void collect() override final;
        void compute() override final;
        void enforce() override final;
        public:
            float const hit_rate_threashold { 0.3f };
            NaiveControlAlgorithm(Cache* cache, std::chrono::milliseconds periodicity) :
                cache(cache), ControlAlgorithm (periodicity)
            {}
            ~NaiveControlAlgorithm() { cache=NULL; }
    };
}
