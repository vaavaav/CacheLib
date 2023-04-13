#pragma once
#include <map>

namespace holpaca::common {
    struct SubStatus {
        uint64_t usedMem;
        uint64_t freeMem;
        uint32_t hits;
        uint32_t lookups;
    };
    
    using Status = std::map<holpaca::Id, SubStatus> ;
}