#pragma once
#include <memory>
#include <holpaca/data_plane/cache.h>

using namespace holpaca;

namespace holpaca {
    namespace data_plane {
        class Stage {
            public:
                virtual ~Stage() = 0;
            protected:
                Stage() = delete;
                Cache* m_cache;
                Stage(Cache* cache) : m_cache(cache) {}
        };

        inline Stage::~Stage() {};
    }
}
