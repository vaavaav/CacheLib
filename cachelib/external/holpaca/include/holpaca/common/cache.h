#pragma once
#include <holpaca/config.h>
#include <holpaca/common/status.h>
#include <memory>


namespace holpaca::common {
    class Cache {
        public:
            // virtual
            virtual void resize(Id srcPool, Id dstPool, size_t delta) = 0; 
            virtual void resize(Id target, size_t newSize) = 0; 
            virtual size_t size() = 0;
            virtual Status getStatus() = 0;
    };
}
