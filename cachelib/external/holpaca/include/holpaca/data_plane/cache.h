#pragma once
#include <holpaca/utils/types.h>
#include <holpaca/config.h>
#include <memory>
#include <map>

namespace holpaca::data_plane {
    class Cache {
        public: 
            virtual void resize(id_t cache_id, size_t new_size) = 0; 
    };
}