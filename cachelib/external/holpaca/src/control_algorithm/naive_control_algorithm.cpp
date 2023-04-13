#include <holpaca/control_algorithm/naive_control_algorithm.h>
#include <cstdint>

namespace holpaca {
    namespace control_algorithm {
        void NaiveControlAlgorithm::collect() { 
            m_logger->info("Collecting cache status");
            if(cache == NULL) {
                m_logger->warn("error while accessing cache object");
                return;
            }
            for (auto const& [id, status] : cache->getStatus()) {
                m_logger->debug("id={} hits={} size={} free={}", id, status.hits, status.usedMem, status.freeMem);
                Stats new_stats = {
                    .hits = status.hits,
                    .size = status.usedMem
                };
                auto [it, success] = stats.insert(std::pair<Id, Stats>(id, new_stats));
                if(!success) {
                    it->second = new_stats;
                }
            };
        }

        void NaiveControlAlgorithm::compute() {
            m_logger->info("Computing best configuration");
            uint32_t giver_hits {0};
            uint32_t taker_hits {UINT32_MAX}; 
            if (stats.size() < 2) {
                m_logger->info("No solution");
                solution = std::nullopt;
                return;
            }
            Solution s {};
            for(auto const& [id, stat] : stats) {
                if( stat.hits > giver_hits) { // maximize
                    giver_hits = stat.hits;
                    s.giver = id;
                } 
                if (stat.hits < taker_hits) { // minimize
                    taker_hits = stat.hits;
                    s.taker = id;
                }
            }
            double const ratio = 0.1 * static_cast<double>(taker_hits) / static_cast<double>(giver_hits);
            s.delta = static_cast<size_t>(stats[s.giver].size * ratio);
            m_logger->debug("Solution: {}->{} (delta:{})", s.giver, s.taker, s.delta);
            solution = s;
        }

        void NaiveControlAlgorithm::enforce() {
            if (cache == NULL) {
                m_logger->warn("error while accessing cache object");
                return;
            }
            if (solution != std::nullopt) {
                m_logger->info("Enforcing solution");
                auto& s = solution.value(); 
                cache->resize(s.giver, s.taker, s.delta);
                m_logger->debug("Giver new size = {}", stats[s.giver].size - s.delta);
                m_logger->debug("Taker new size = {}", stats[s.taker].size + s.delta);
            }
        }
    }
}
