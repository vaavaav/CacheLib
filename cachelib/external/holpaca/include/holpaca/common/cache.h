#pragma once
#include <holpaca/config.h>
#include <holpaca/common/status.h>
#include <memory>


namespace holpaca::common {
    class Cache {
        public:
            // virtual
            virtual void resize(Id srcPool, Id dstPool, size_t delta) = 0; 
            virtual Status getStatus() = 0;
    };
}
