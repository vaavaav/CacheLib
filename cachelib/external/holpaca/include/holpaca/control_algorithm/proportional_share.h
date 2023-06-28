#pragma once
#include <holpaca/control_algorithm/control_algorithm.h> 
#include <holpaca/common/cache.h>
#include <unordered_map>
#include <algorithm>
#include <optional>

using namespace holpaca::common;

namespace holpaca::control_algorithm {
    class ProportionalShare : public ControlAlgorithm {
        ProportionalShare() = delete;
        Cache* cache;
        std::vector<id_t> pool_ids;
        int left {0};
        std::unordered_map<id_t, int> rate;
        std::vector<id_t> inactive;

        void collect() override final {
            m_logger->info("Collecting cache status");
            if(cache == NULL) {
                m_logger->warn("error while accessing cache object");
                return;
            }
            pool_ids.clear();
            inactive.clear();
            rate.clear();
            left = cache->size();
            for (auto const& [pid, status] : cache->getStatus()) {
                if(status.isActive){
                    pool_ids.push_back(pid);
                    m_logger->info("pool={} is active, curr size={}", pid, status.usedMem);
                } else {
                    inactive.push_back(pid);
                }
            }
        }

        void compute() override final {
            for (auto const& pid : pool_ids){
                rate[pid] = left / (static_cast<int>(pid)+2);
                m_logger->info("pool={} rate={}", pid, rate[pid]);
                left -= rate[pid];
                m_logger->info("left={}", left);
            }
            for (auto const& pid : pool_ids){
                rate[pid] += left / pool_ids.size();
            }
        }
        void enforce() override final {
            for (auto const& [pool_id, new_size] : rate) {
                cache->resize(pool_id, new_size);
                m_logger->info("pool={} newsize={}", pool_id, new_size);
            }
            for (auto const& pid : inactive) {
                cache->resize(pid, 0);
            }
        }

        public:
            ProportionalShare(Cache* cache, std::chrono::milliseconds periodicity) :
                cache(cache), ControlAlgorithm (periodicity)
            {}
            ~ProportionalShare() {
                m_logger->info("Destructing ProportionalShare");
                stop();
            }
    };
}