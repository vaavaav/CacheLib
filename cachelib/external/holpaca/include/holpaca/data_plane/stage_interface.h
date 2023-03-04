#pragma once
#include <holpaca/utils/snapshot.h>
#include <holpaca/utils/types.h>
#include <vector>

namespace holpaca::data_plane {
    class StageInterface {
        public: 
            virtual void resizeCache(id_t cache_id, size_t const new_size) = 0; 
            virtual std::vector<snapshot_t> getFullSnapshot() = 0;
    };
}