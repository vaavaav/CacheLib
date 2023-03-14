#include <holpaca/control_algorithm/naive_control_algorithm.h>
#include <cstdint>

namespace holpaca {
    namespace control_algorithm {
        void NaiveControlAlgorithm::collect() { 
            m_logger->info("Collecting cache status");
            try { 
                auto const& status = cache->getStatus();
                for (auto const& [id, status] : status) {
                    m_logger->debug("id={} hits={} size={}", id, status.size, status.hits);
                    Stats new_stats = {
                        .hits = status.hits,
                        .size = status.size
                    };
                     auto [it, _] = stats.insert(std::pair<Id, Stats>(id, new_stats));
                     if(!it->first) {
                         it->second = new_stats;
                     }
                };
            } catch (...) {
                m_logger->warn("error while accessing cache object");
                return;
            }
        }

        void NaiveControlAlgorithm::compute() {
            m_logger->info("Computing best configuration");
            uint32_t giver_hits {0};
            uint32_t taker_hits {UINT32_MAX}; 
            bool has_giver {false};
            bool has_taker {false};
            Solution s {};
            for(auto const& [id, stat] : stats) {
                if( stat.hits > giver_hits) { // maximize
                    giver_hits = stat.hits;
                    s.giver = id;
                    has_giver = true;
                } else if (stat.hits < taker_hits) { // minimize
                    taker_hits = stat.hits;
                    s.taker = id;
                    has_taker = true;
                }
            }
            if (has_giver && has_taker){
                s.delta = static_cast<size_t>(
                    stats[s.giver].size * 0.01 * (static_cast<float>(taker_hits) / static_cast<float>(giver_hits))
                );
                m_logger->debug("Solution: {}->{} (delta:{})", s.giver, s.taker, s.delta);
                solution = s;
            } else {
                solution = std::nullopt;
            }
        }

        void NaiveControlAlgorithm::enforce() {
            if (solution != std::nullopt) {
                m_logger->info("Enforcing solution");
                auto& s = solution.value(); 
                try {
                    cache->resize(s.giver, stats[s.giver].size - s.delta);
                    m_logger->debug("Giver new size = {}", stats[s.giver].size - s.delta);
                    cache->resize(s.taker, stats[s.taker].size + s.delta);
                    m_logger->debug("Taker new size = {}", stats[s.taker].size + s.delta);
                } 
                catch (...) {
                    m_logger->warn("error while accessing cache object");
                    return;
                }
            }
        }
    }
}
