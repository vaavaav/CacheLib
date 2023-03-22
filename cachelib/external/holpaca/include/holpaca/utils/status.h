#pragma once
#include <cstddef>
#include <cstdint>

namespace holpaca {
    struct Status {
        std::size_t const size;
        std::size_t const free;
        std::uint32_t const lookups;
        std::uint32_t const hits;
    };
}
