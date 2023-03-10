#pragma once
#include <memory>
#include <holpaca/data_plane/cache.h>

using namespace holpaca;

namespace holpaca::data_plane {
    class Stage {
        Stage() = delete;
        protected:
            Cache* m_cache;
        public:
            Stage(Cache* cache) : m_cache(cache) {}
        
    };
}