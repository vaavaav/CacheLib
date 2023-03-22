#pragma once
#include <holpaca/utils/types.h>
#include <holpaca/utils/status.h>
#include <memory>
#include <map>

using namespace holpaca;

namespace holpaca::data_plane {
    class Cache {
        public:
            virtual void resize(Id cache_id, size_t new_size) = 0; 
            virtual void resize(Id src, Id dst, size_t delta) = 0; 
            virtual std::map<Id, Status> getStatus() = 0;
    };
}
