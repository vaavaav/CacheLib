#pragma once
#include <holpaca/control_algorithm/control_algorithm.h> 
#include <holpaca/common/cache.h>
#include <unordered_map>
#include <algorithm>
#include <optional>

using namespace holpaca::common;

namespace holpaca::control_algorithm {
    class MarginalHits : public ControlAlgorithm {
        MarginalHits() = delete;
        Cache* cache;
        
        double const movingAverageParam {0.3};
        uint32_t const poolMinSizeSlabs{1 << 22};
        uint32_t const poolMaxFreeSlabs{2 * (1 << 22)};
        std::unordered_map<uint64_t, double> smoothedRanks;
        std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint32_t>> accumTailHits;
        std::unordered_map<uint64_t, std::unordered_map<uint64_t, uint32_t>> tailHits;
        std::unordered_map<uint64_t, bool> validReceiver;
        std::unordered_map<uint64_t, bool> validVictim;
        std::unordered_map<uint64_t, uint32_t> score;
        std::vector<uint64_t> poolIds;
        std::optional<uint64_t> victim = std::nullopt;
        std::optional<uint64_t> receiver = std::nullopt;

        void collect() override final {
            m_logger->info("Collecting cache status");
            poolIds.clear();
            validReceiver.clear();
            validVictim.clear();
            tailHits.clear();
            if(cache == NULL) {
                m_logger->warn("error while accessing cache object");
                return;
            }
            for (auto const& [id, status] : cache->getStatus()) {
                m_logger->debug("id={} evictions={} hits={} size={} free={}", id, status.evictions, status.hits, status.usedMem, status.freeMem);
                validVictim[id] = status.evictions > 0 && status.usedMem > poolMinSizeSlabs;
                validReceiver[id] = status.freeMem < poolMaxFreeSlabs;
                poolIds.push_back(id);
                for (auto const& [cid, tailAccesses] : status.tailAccesses) {
                    m_logger->debug("tailAccesses[{}][{}] = {}", id, cid, tailAccesses);
                    accumTailHits[id][cid] = 0;
                    tailHits[id][cid] = tailAccesses - accumTailHits[id][cid];
                    accumTailHits[id].find(cid)->second = tailAccesses;
                }
            }
        }
        void compute() override final {
            if (poolIds.size() == 0) {
                victim = std::nullopt;
                receiver = std::nullopt;
                return;
            }
            score.clear();
            for (auto const& pid : poolIds) {
                smoothedRanks[pid] = 0;
                for(auto const& [cid, cTailHits] : tailHits[pid]) {
                    score[pid] = std::max(score[pid], cTailHits);
                }
            }
            auto cmp = [&](auto x, auto y) { return score[x] < score[y]; };
            std::sort(poolIds.begin(), poolIds.end(), cmp);
            for(uint32_t i = 0; i < poolIds.size(); i++) {
                auto avg = &smoothedRanks[poolIds[i]];
                (*avg) = (*avg) * movingAverageParam + i * (1 - movingAverageParam);
                m_logger->debug("smoothedRanks[{}] = {}", poolIds[i], smoothedRanks[poolIds[i]]);
            }
            double minRank = smoothedRanks.size();
            double maxRank = -1;
            for (auto const& [pid, rank] : smoothedRanks) {
                if (validReceiver[pid] && rank > maxRank) {
                    maxRank = rank;
                    receiver = pid;
                }
                if (validVictim[pid] && rank < minRank) {
                    minRank = rank;
                    victim = pid;
                }
            }
        }
        void enforce() override final {
            if(receiver != std::nullopt && victim != std::nullopt) {
                m_logger->info("enforcing: victim={} receiver={} 1 slab", victim.value(), receiver.value());
                cache->resize(victim.value(), receiver.value(), 1 << 22);
            }
        }

        public:
            MarginalHits(Cache* cache, std::chrono::milliseconds periodicity) :
                cache(cache), ControlAlgorithm (periodicity)
            {}
            ~MarginalHits() {
                m_logger->info("Destructing MarginalHits");
                stop();
            }
    };
}