#include <holpaca/control_algorithm/naive_control_algorithm.h>

namespace holpaca::control_algorithm {
    void NaiveControlAlgorithm::collect() {
       auto const& status = cache->getStatus();
       SPDLOG_LOGGER_DEBUG(m_logger, "collect()");
       for (auto const& [id, status] : status) {
            SPDLOG_LOGGER_DEBUG(m_logger, "got id={}", id);
            SPDLOG_LOGGER_DEBUG(m_logger, "size={}", status.size);
            SPDLOG_LOGGER_DEBUG(m_logger, "hits={}", status.hits);
            auto [it, _] = stats.insert(std::pair<Id, Stats>(id, {.hits = status.hits, .size = status.size}));
            it->second = {
                .hits = status.hits,
                .size = status.size
            };
       };
    }
    void NaiveControlAlgorithm::compute() {
        SPDLOG_LOGGER_DEBUG(m_logger, "compute()");
        if (stats.size() == 0) {
            solution = std::nullopt;
            return;
        }
        uint32_t giver_hits = 0;
        bool has_giver {false};
        Solution s = {};
        for(auto const& [id, stat] : stats) {
            SPDLOG_LOGGER_DEBUG(m_logger, "processing id={}, hits={}, max={}", id, stat.hits, giver_hits);
            if( stat.hits > giver_hits) {
                has_giver = true;
                giver_hits = stat.hits;
                s.giver = id;
            } 
        }
        uint32_t taker_hits = giver_hits;
        s.taker = s.giver;
        bool has_taker {false};
        for(auto const& [id, stat] : stats) {
            SPDLOG_LOGGER_DEBUG(m_logger, "processing id={}, hits={}, min={}", id, stat.hits, taker_hits);
            if (stat.hits < taker_hits) {
                has_taker = true;
                taker_hits = stat.hits;
                s.taker = id;
            }
        }
        if (has_giver && has_taker) {
            SPDLOG_LOGGER_DEBUG(m_logger, "has solution");
            s.delta = static_cast<size_t>(
                stats[s.giver].size * 0.1 * (static_cast<float>(taker_hits) / static_cast<float>(giver_hits))
            );
            SPDLOG_LOGGER_DEBUG(m_logger, "delta={}", s.delta);
            SPDLOG_LOGGER_DEBUG(m_logger, "delta should be={}", stats[s.giver].size * 0.1 * (static_cast<float>(taker_hits) / static_cast<float>(giver_hits)));
            solution = s;
        }
    }

    void NaiveControlAlgorithm::enforce() {
        SPDLOG_LOGGER_DEBUG(m_logger, "enforce()");
        if (solution != std::nullopt) {
            auto& s = solution.value(); 
            SPDLOG_LOGGER_DEBUG(m_logger, "giver={}", s.giver);
            SPDLOG_LOGGER_DEBUG(m_logger, "taker={}", s.taker);
            cache->resize(s.giver, stats[s.giver].size - s.delta);
            SPDLOG_LOGGER_DEBUG(m_logger, "giver new size={}", stats[s.giver].size - s.delta);
            cache->resize(s.taker, stats[s.taker].size + s.delta);
            SPDLOG_LOGGER_DEBUG(m_logger, "taker new size={}", stats[s.taker].size + s.delta);
        }
    }
}
