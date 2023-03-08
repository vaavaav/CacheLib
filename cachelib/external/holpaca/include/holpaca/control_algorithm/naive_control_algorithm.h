#pragma once
#include <holpaca/control_algorithm/control_algorithm.h>
#include <holpaca/utils/types.h>

namespace holpaca::control_algorithm {
    class NaiveCA : public ControlAlgorithm {
        void collect() override final;
        void compute() override final;
        void enforce() override final;
    };
}