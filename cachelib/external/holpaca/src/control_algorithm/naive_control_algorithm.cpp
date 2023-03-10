#include <holpaca/control_algorithm/naive_control_algorithm.h>

namespace holpaca::control_algorithm {
    void NaiveControlAlgorithm::collect() {
       for (auto const& [id, status] : cache->getStatus()) {
            stats[id] = {
                .hits = status.hits,
                .size = status.size
            };
       };
    }
    void NaiveControlAlgorithm::compute() {
        uint32_t giver_hits = 0;
        uint32_t taker_hits = 0;
        for(auto const& [id, stat] : stats) {
            if( stat.hits > giver_hits) {
                giver_hits = stat.hits;
                solution.giver = id;
            } else if (stat.hits < taker_hits) {
                taker_hits = stat.hits;
                solution.taker = id;
            }
        }
        solution.delta = static_cast<size_t>(
            stats[solution.giver].size * (giver_hits - taker_hits)
        );
    }
    void NaiveControlAlgorithm::enforce() {
        cache->resize(solution.giver, stats[solution.giver].size - solution.delta);
        cache->resize(solution.taker, stats[solution.taker].size + solution.delta);
    }
}