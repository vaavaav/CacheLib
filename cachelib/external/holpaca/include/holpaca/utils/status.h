#pragma once
#include <cstddef>
#include <cstdint>

namespace holpaca {
    struct Status {
        std::size_t const size;
        std::uint32_t const lookups;
        std::uint32_t const hits;
    };
}