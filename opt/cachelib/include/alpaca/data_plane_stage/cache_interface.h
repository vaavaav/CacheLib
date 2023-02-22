#pragma once
#include <cstddef>

namespace alpaca::data_plane {
    class CacheInterface {

        protected:
            virtual size_t size() const = 0;
            virtual void resize(size_t const& new_size) = 0;
            virtual size_t max_size() const = 0;
    };
}
