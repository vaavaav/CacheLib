#include <holpaca/data_plane/autonomous_stage.h>

namespace holpaca::data_plane {
    AutonomousStage::AutonomousStage(
        std::shared_ptr<Cache> cache,
        char const* log_file 
    ) {
        this->cache = cache;
    }
}