#pragma once
#include <holpaca/common/cache.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

extern "C" {
#include <gsl/gsl_siman.h>
};

using namespace holpaca::common;

namespace holpaca::control_algorithm {

#define OBJECT_SIZE 1847
#define MAX_DELTA 0.05
#define QOS_MARGIN 0.01
#define MRC_MIN_SIZE 3

struct Context {
  double m_max{0};
  struct PoolConfig {
    double m_size{0};
    double m_current_size{0};

    std::map<double, double> m_mrc{};
    double m_cache_size_for_qos_lower_bound{0};
    double m_cache_size_for_qos_upper_bound{0};
    double getCacheSizeForMR(double const mr) const {
      if (mr >= m_mrc.begin()->second) {
        return m_mrc.begin()->first * mr / m_mrc.begin()->second;
      }
      for (auto i = m_mrc.begin(), j = std::next(i); j != m_mrc.end();
           i++, j++) {
        if (j->second <= mr && mr <= i->second) {
          double const r = (mr - j->second) / (i->second - j->second);
          return j->first * (1 - r) + i->first * r;
        }
      }
      return (m_mrc.rbegin()->first) * mr / m_mrc.rbegin()->second;
    };

    double getMR() const {
      if (m_size < m_mrc.begin()->first) {
        double const r = m_size / m_mrc.begin()->first;
        return (1 - r) + m_mrc.begin()->second * r;
      }
      for (auto i = m_mrc.begin(), j = std::next(i); j != m_mrc.end();
           i++, j++) {
        if (i->first <= m_size && m_size <= j->first) {
          double const r = (m_size - i->first) / (j->first - i->first);
          return i->second * (1 - r) + j->second * r;
        }
      }
      return (m_mrc.rbegin()->first) * (m_mrc.rbegin()->second) / m_size;
    };
  };
  std::unordered_map<pid_t, PoolConfig> m_pool_configs{};
};

class MissRateMin {
  MissRateMin() = delete;
  Cache* cache = NULL;
  std::shared_ptr<spdlog::logger> m_logger;
  std::unordered_map<pid_t, double> m_qos;
  std::unordered_set<pid_t> m_active{};
  std::unordered_set<pid_t> m_inactive{};
  const gsl_rng_type* T;
  gsl_rng* r;
  Context ctx{};

 public:
  void collect() {
    if (cache == NULL) {
      return;
    }
    m_logger->info("Collecting cache status");

    double const max = cache->size() / static_cast<double>(OBJECT_SIZE);
    if (max == 0) {
      return;
    }

    auto const& statuses = cache->getStatus();
    int const number_of_active_pools =
        std::count_if(statuses.begin(), statuses.end(),
                      [](auto const& s) { return s.second.isActive; });
    bool const pools_join_or_left =
        std::any_of(statuses.begin(), statuses.end(), [&](auto const& s) {
          return s.second.isActive ==
                 (m_active.find(s.first) == m_active.end());
        });
    int const number_of_warming_pools = std::count_if(
        statuses.begin(), statuses.end(), [&](auto const& s) -> bool {
          return s.second.isActive && (s.second.mrc.size() < MRC_MIN_SIZE);
        });

    auto const default_size = max / number_of_active_pools;
    ctx.m_max = max - number_of_warming_pools * default_size;
    m_logger->debug("Cache max size: {} (objects of {} MB)", max, OBJECT_SIZE);
    m_logger->debug("Default pool size: {}", default_size);
    m_logger->debug("{} of {} active pools are warming up",
                    number_of_warming_pools, number_of_active_pools);

    // first I need to resize all inactive to 0 so that there is free available
    // space for the active ones
    for (auto const& [id, pool] : statuses) {
      if (!pool.isActive) {
        if (ctx.m_pool_configs.find(id) != ctx.m_pool_configs.end()) {
          ctx.m_pool_configs.erase(id);
        }
        if (m_active.find(id) != m_active.end()) {
          m_active.erase(id);
        }
        if (m_inactive.find(id) == m_inactive.end()) {
          m_inactive.insert(id);
          // preemptive enforce
          cache->resize(id, 0);
        }
      }
    }
    for (auto const& [id, pool] : statuses) {
      if (pool.isActive) {
        m_active.insert(id);
        if (pools_join_or_left) {
          // preemptive enforce
          cache->resize(id, default_size * OBJECT_SIZE);
        }
        if (m_inactive.find(id) != m_inactive.end()) {
          m_inactive.erase(id);
        }
        if (pool.mrc.size() >= MRC_MIN_SIZE) {
          Context::PoolConfig pc{};
          pc.m_current_size = pool.usedMem / static_cast<double>(OBJECT_SIZE);
          pc.m_size = pc.m_current_size;
          pc.m_mrc = {pool.mrc.begin(), pool.mrc.end()};
          if (auto const qos = m_qos.find(id); qos == m_qos.end()) {
            pc.m_cache_size_for_qos_lower_bound = max;
            pc.m_cache_size_for_qos_upper_bound = 0.0;
          } else {
            auto mr = pc.getMR();
            if (qos->second - QOS_MARGIN <= mr &&
                mr <= qos->second + QOS_MARGIN) {
              ctx.m_max -= pc.m_current_size;
              ctx.m_pool_configs.erase(id);
            } else {
              pc.m_cache_size_for_qos_lower_bound =
                  pc.getCacheSizeForMR(qos->second - QOS_MARGIN);
              pc.m_cache_size_for_qos_upper_bound =
                  pc.getCacheSizeForMR(qos->second + QOS_MARGIN);
              ctx.m_pool_configs[id] = pc;
            }
          }
          m_logger->debug("pool #{} (size: {}) (mr: {}) (bounds: [{},{}])", id,
                          pc.m_size, pc.getMR(),
                          pc.m_cache_size_for_qos_upper_bound,
                          pc.m_cache_size_for_qos_lower_bound);
        }
      }
    }
    m_logger->debug(
        "Available cache space for optimization: {} (objects of {} MB)",
        ctx.m_max, OBJECT_SIZE);
  }

  void compute() {
    if (cache == NULL || ctx.m_pool_configs.size() < 2) {
      return;
    }
    m_logger->debug("Computing for {} pools", ctx.m_pool_configs.size());
    gsl_siman_params_t params = {
        .n_tries = 2000,
        .iters_fixed_T = 150,
        .step_size = 10.0, // not used
        .k = 1.0,
        .t_initial = 90,
        .mu_t = 1.003,
        .t_min = 0.1,
    };

    gsl_siman_solve(r, &ctx, E1, S1, M1, NULL, NULL, NULL, NULL, (sizeof ctx),
                    params);
  }

  void enforce() {
    if (cache == NULL || ctx.m_pool_configs.size() < 2) {
      return;
    }
    m_logger->debug("Accum Miss Rate: {} ", E1(&ctx));
    std::vector<double> sizes_sorted_by_diff{};
    for (auto const& [id, _] : ctx.m_pool_configs) {
      sizes_sorted_by_diff.push_back(id);
    }
    std::sort(sizes_sorted_by_diff.begin(), sizes_sorted_by_diff.end(),
              [&](auto const& a, auto const& b) {
                return (ctx.m_pool_configs[a].m_size -
                        ctx.m_pool_configs[a].m_current_size) <
                       (ctx.m_pool_configs[b].m_size -
                        ctx.m_pool_configs[b].m_current_size);
              });
    for (auto& id : sizes_sorted_by_diff) {
      auto& pc = ctx.m_pool_configs[id];
      if (pc.m_current_size != pc.m_size) {
        m_logger->debug("pool #{}: {} -> {}", id, pc.m_current_size, pc.m_size);
        cache->resize(id, pc.m_size * OBJECT_SIZE);
      }
    }
  }

  static void S1(const gsl_rng* r, void* xp, double step_size) {
    auto x = ((Context*)xp);
    auto const n = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    auto m = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    while (n == m) {
      m = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    }
    auto receiver = x->m_pool_configs.begin();
    auto giver = x->m_pool_configs.begin();
    std::advance(receiver, n);
    std::advance(giver, m);

    double const min_delta = std::max({
        0.0,
        receiver->second.m_cache_size_for_qos_upper_bound -
            receiver->second.m_current_size,
        giver->second.m_current_size -
            giver->second.m_cache_size_for_qos_lower_bound,
    });

    double const max_delta =
        std::min({giver->second.m_current_size -
                      giver->second.m_cache_size_for_qos_upper_bound,
                  receiver->second.m_cache_size_for_qos_lower_bound -
                      receiver->second.m_current_size,
                  giver->second.m_current_size,
                  x->m_max - receiver->second.m_current_size, MAX_DELTA * x->m_max});
    if (max_delta < min_delta) {
      return;
    }
    double const delta =
        gsl_rng_uniform(r) * (max_delta - min_delta) + min_delta;

    receiver->second.m_size = receiver->second.m_current_size + delta;
    giver->second.m_size = giver->second.m_current_size - delta;
  }

  /* now some functions to test in one dimension */
  static double E1(void* xp) {
    auto x = (Context*)xp;
    return std::accumulate(
        x->m_pool_configs.begin(), x->m_pool_configs.end(), 0.0,
        [&](auto const& a, auto const& b) { return a + b.second.getMR(); });
  }

  static double M1(void* xp, void* yp) { return fabs(E1(xp) - E1(yp)); }

 public:
  void run() {
    collect();
    compute();
    enforce();
  }
  MissRateMin(Cache* cache, std::unordered_map<pid_t, double> qos)
      : cache(cache), m_qos(qos) {
    try {
      m_logger =
          spdlog::basic_logger_st("missratemin", "/tmp/missratemin.log", true);
    } catch (const spdlog::spdlog_ex& ex) {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
      abort();
    }
    m_logger->flush_on(spdlog::level::trace);
    m_logger->set_level(spdlog::level::debug);
    m_logger->info("Initialization");
    gsl_rng_env_setup();
    r = gsl_rng_alloc(gsl_rng_default);
  }

  ~MissRateMin() {
    m_logger->info("Destructing MissRateMin");
    gsl_rng_free(r);
  }
};
} // namespace holpaca::control_algorithm