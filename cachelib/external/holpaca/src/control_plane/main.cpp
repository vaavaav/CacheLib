#include <holpaca/control_plane/cache_proxy.h>
#include <iostream>

int main() {
    holpaca::control_plane::CacheProxy cp;

    auto s = cp.getStatus();

    for( auto const& [id, status] : s) {
        std::cout << "id=" << id << " hits=" << status.hits << " size=" << status.usedMem << " free=" << status.freeMem << std::endl;
    }

    return 0;
}