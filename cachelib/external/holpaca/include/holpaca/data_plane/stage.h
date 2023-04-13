#pragma once
#include <holpaca/common/cache.h>

using holpaca::common::Cache;

namespace holpaca::data_plane {
    class Stage {
        public:
            Stage() = delete;
            virtual ~Stage() = 0;

        protected:
            Cache* const cache;
            Stage(Cache* const cache) : cache(cache) {}
    };
    
    inline Stage::~Stage() {}
}