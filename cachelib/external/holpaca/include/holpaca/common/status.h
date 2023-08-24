#pragma once
#include <map>

namespace holpaca::common {
    struct SubStatus {
        uint64_t usedMem;
        uint64_t freeMem;
        uint32_t hits;
        uint32_t lookups;
        uint64_t evictions;
        std::map<uint64_t, uint32_t> tailAccesses;
        std::map<uint32_t, double> mrc;
        bool isActive;
    };
    
    using Status = std::map<holpaca::Id, SubStatus> ;
}