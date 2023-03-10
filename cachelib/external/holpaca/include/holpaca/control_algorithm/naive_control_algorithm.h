#pragma once
#include <holpaca/control_algorithm/control_algorithm.h>
#include <holpaca/data_plane/cache.h>
#include <holpaca/utils/types.h>
#include <cstddef>
#include <unordered_map>

using namespace holpaca::data_plane;

namespace holpaca::control_algorithm {
    class NaiveControlAlgorithm : public ControlAlgorithm {
        NaiveControlAlgorithm() = delete;
        std::shared_ptr<Cache> cache;

        struct stats {
            uint32_t hits;
            size_t size; 
        };
        std::unordered_map<id_t, stats> stats;
        struct solution {
            Id giver;
            size_t delta;
            Id taker;
        };
        solution solution;
        void collect() override final;
        void compute() override final;
        void enforce() override final;
        public:
            float const hit_rate_threashold { 0.3f };
            NaiveControlAlgorithm(std::shared_ptr<Cache> cache) :
                ControlAlgorithm { 1s }
            {}
    };
}