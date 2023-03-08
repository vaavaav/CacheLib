#pragma once
#include <holpaca/data_plane/cache.h>

namespace holpaca::data_plane {
    class Stage {
        protected:
            std::shared_ptr<Cache> cache;
    };
}