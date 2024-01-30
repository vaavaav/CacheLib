#pragma once
#include <holpaca/common/cache.h>
#include <holpaca/control_algorithm/control_algorithm.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <numeric>
#include <unordered_map>
#include <unordered_set>

#include "spline.h"
extern "C" {
#include <gsl/gsl_siman.h>
};

using namespace holpaca::common;

namespace holpaca::control_algorithm {

#define MAX_DELTA 0.05
#define QOS_MARGIN 0.01
#define MRC_MIN_SIZE 5

class MissRateMinQOS : public ControlAlgorithm {
  struct Context {
    double m_max{0};
    struct PoolConfig {
      double m_size{0};
      double m_current_size{0};
      bool m_override_max_delta{false};
      tk::spline m_mrc;
      double m_cache_size_for_qos_lower_bound{0};
      double m_cache_size_for_qos_upper_bound{0};
      double inline getMR() const { return getMR(m_size); };
      double inline getMR(double const size) const { return m_mrc(size); };
    };
    std::unordered_map<pid_t, PoolConfig> m_pool_configs{};
  };

  MissRateMinQOS() = delete;
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

    double const max = cache->size();
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
    m_logger->debug("Cache max size: {} byte", max);
    m_logger->debug("Default pool size: {} byte", default_size);
    m_logger->debug("{} of {} active pools are warming up",
                    number_of_warming_pools, number_of_active_pools);

    // first I need to resize all inactive to 0 so that there is free available
    // space for the active ones
    for (auto const& [id, pool] : statuses) {
      if (!pool.isActive) {
        ctx.m_pool_configs.erase(id);
        m_active.erase(id);
        if (m_inactive.find(id) == m_inactive.end()) {
          m_inactive.insert(id);
          // preemptive enforce
          cache->resize(id, 0);
        }
      }
    }
    // if pools join or left do a preemptive resize (from the most decreased
    // pool to the most increased one)
    if (pools_join_or_left) {
      std::vector<double> pools{};
      std::unordered_map<pid_t, double> sizes{};
      for (auto const& [id, pool] : statuses) {
        if (pool.isActive) {
          pools.push_back(id);
          sizes[id] = pool.usedMem;
        }
      }
      // sort pools by difference between current size and default size
      std::sort(pools.begin(), pools.end(), [&](auto const& a, auto const& b) {
        return sizes[a] > sizes[b];
      });
      for (auto const& id : pools) {
        cache->resize(id, default_size);
      }
    }

    for (auto const& [id, pool] : statuses) {
      m_logger->debug("pool #{}, mean object size={}", id, pool.meanObjectSize);
      if (pool.isActive) {
        m_active.insert(id);
        m_inactive.erase(id);
        if (pool.mrc.size() >= MRC_MIN_SIZE) {
          Context::PoolConfig pc{};
          pc.m_current_size = pools_join_or_left ? default_size : pool.usedMem;
          if (pool.meanObjectSize == 0) {
            ctx.m_pool_configs.erase(id);
            ctx.m_max -= pc.m_current_size;
            continue;
          }
          pc.m_size = pc.m_current_size;
          auto mrc = std::map<double, double>{pool.mrc.begin(), pool.mrc.end()};
          std::vector<double> cache_sizes;
          cache_sizes.reserve(pool.mrc.size());
          std::vector<double> miss_rates;
          miss_rates.reserve(pool.mrc.size());
          for (auto const& [cs, mr] : mrc) {
            cache_sizes.push_back(cs * pool.meanObjectSize);
            miss_rates.push_back(mr);
          }
          pc.m_mrc =
              tk::spline(cache_sizes, miss_rates, tk::spline::cspline, true);
          double const real_mr =
              1.0 - static_cast<double>(pool.hits) / pool.lookups;
          double const shift_mr = real_mr - pc.m_mrc(pool.usedMem);
          for (auto& mr : miss_rates) {
            mr += shift_mr;
          }
          pc.m_mrc =
              tk::spline(cache_sizes, miss_rates, tk::spline::cspline, true);
          if (auto const qos = m_qos.find(id); qos == m_qos.end()) {
            pc.m_cache_size_for_qos_upper_bound = max;
            pc.m_cache_size_for_qos_lower_bound = 0.0;
            ctx.m_pool_configs[id] = pc;
          } else {
            pc.m_override_max_delta = true;
            if (qos->second - QOS_MARGIN <= real_mr &&
                real_mr <= qos->second + QOS_MARGIN) {
              ctx.m_pool_configs.erase(id);
              ctx.m_max -= pc.m_current_size;
            } else {
              pc.m_cache_size_for_qos_upper_bound =
                  qos->second > real_mr ? (1 - MAX_DELTA) * pc.m_current_size
                                        : max;
              pc.m_cache_size_for_qos_lower_bound =
                  qos->second < real_mr ? (1 + MAX_DELTA) * pc.m_current_size
                                        : 0.0;
              ctx.m_pool_configs[id] = pc;
            }
          }
        }
      }
    }
  }

  void compute() {
    if (cache == NULL || ctx.m_pool_configs.size() < 2) {
      return;
    }

    for (auto const& [id, pc] : ctx.m_pool_configs) {
      m_logger->debug("pool #{} (size: {}) (mr: {}) (bounds: [{},{}])", id,
                      pc.m_size, pc.getMR(),
                      pc.m_cache_size_for_qos_lower_bound,
                      pc.m_cache_size_for_qos_upper_bound);
    }

    m_logger->debug("Computing for {} pools", ctx.m_pool_configs.size());
    gsl_siman_params_t params = {
        .n_tries = 2000,
        .iters_fixed_T = 250,
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
    for (auto const& id : sizes_sorted_by_diff) {
      auto const& pc = ctx.m_pool_configs[id];
      if (pc.m_current_size != pc.m_size) {
        m_logger->debug("pool #{}: {} -> {}", id, pc.m_current_size, pc.m_size);
        cache->resize(id, pc.m_size);
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

    double const min_delta = std::max(
        {receiver->second.m_cache_size_for_qos_lower_bound -
             receiver->second.m_size,
         giver->second.m_size - giver->second.m_cache_size_for_qos_upper_bound,
         0.0});

    double const max_delta = std::min(
        giver->second.m_size - giver->second.m_cache_size_for_qos_lower_bound,
        receiver->second.m_cache_size_for_qos_upper_bound -
            receiver->second.m_size);

    if (max_delta < min_delta) {
      return;
    }

    double const delta =
        min_delta > 0
            ? min_delta
            : (giver->second.m_override_max_delta ||
                       receiver->second.m_override_max_delta
                   ? std::max(0.0,
                              std::min({gsl_rng_uniform(r) * max_delta,
                                        giver->second.m_size,
                                        x->m_max - receiver->second.m_size}))
                   : std::max(
                         0.0,
                         std::min({gsl_rng_uniform(r) * max_delta,
                                   (1 + MAX_DELTA) *
                                           receiver->second.m_current_size -
                                       receiver->second.m_size,
                                   giver->second.m_size -
                                       (1 - MAX_DELTA) *
                                           giver->second.m_current_size,
                                   giver->second.m_size,
                                   x->m_max - receiver->second.m_size})));

    receiver->second.m_size += delta;
    giver->second.m_size -= delta;
  }

  /* now some functions to test in one dimension */
  static double E1(void* xp) {
    auto x = (Context*)xp;
    return std::accumulate(
        x->m_pool_configs.cbegin(), x->m_pool_configs.cend(), 0.0,
        [&](auto const& a, auto const& b) { return a + b.second.getMR(); });
  }

  static double M1(void* xp, void* yp) { return fabs(E1(xp) - E1(yp)); }

 public:
  void run() override final {
    collect();
    compute();
    enforce();
  }
  MissRateMinQOS(Cache* cache, std::unordered_map<pid_t, double> qos)
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

  ~MissRateMinQOS() {
    m_logger->info("Destructing MissRateMin");
    gsl_rng_free(r);
  }
};
} // namespace holpaca::control_algorithm