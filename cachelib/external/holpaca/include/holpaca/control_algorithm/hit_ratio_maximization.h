#pragma once
#include <holpaca/common/cache.h>
#include <holpaca/control_algorithm/control_algorithm.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
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

struct Context {
  struct PoolCtx {
    double partitioning{0};
    double currentPartitioning{0};
    tk::spline MRC;
  };
  std::unordered_map<pid_t, PoolCtx> m_pool_configs{};
  double delta_max{0.05};
};

class HitRatioMaximization : public ControlAlgorithm {
  size_t const static kMRCMinLength = 3;
  HitRatioMaximization() = delete;
  Cache* cache = NULL;
  std::shared_ptr<spdlog::logger> m_logger;
  Status m_status{};
  uint64_t m_max{0};
  std::unordered_map<pid_t, tk::spline> m_mrcs{};
  const gsl_rng_type* T;
  gsl_rng* r;
  Context ctx{};

  static bool is_active_not_warming(SubStatus const& pool) {
    return pool.isActive && pool.mrc.size() >= kMRCMinLength &&
           pool.meanObjectSize > 0 && pool.usedMem > 0;
  }

  static bool became_active(SubStatus const& pool) {
    return pool.isActive && pool.usedMem == 0;
  }

  static bool became_inactive(SubStatus const& pool) {
    return !pool.isActive && pool.usedMem > 0;
  }

 public:
  void collect() {
    if (cache == NULL) {
      m_logger->warn("Cache is NULL. Skipping collect.");
      return;
    }
    m_logger->info("Collecting cache status");

    m_max = cache->size();
    m_logger->info("Cache size: {}", m_max);
    m_status = cache->getStatus();
    for (auto const& [id, pool] : m_status) {
      if (is_active_not_warming(pool)) {
        std::vector<double> cache_sizes;
        cache_sizes.reserve(pool.mrc.size());
        std::vector<double> miss_rates;
        miss_rates.reserve(pool.mrc.size());
        for (auto const& [cs, mr] : pool.mrc) {
          cache_sizes.push_back(cs * pool.meanObjectSize);
          miss_rates.push_back(mr);
        }
        m_mrcs[id] = tk::spline(cache_sizes, miss_rates, tk::spline::cspline,
                                true, tk::spline::second_deriv, 0.0);
      }
    }
  }

  void compute() {
    auto const number_of_active_pools =
        std::count_if(m_status.begin(), m_status.end(),
                      [](auto const& s) { return s.second.isActive; });
    auto const tenants_left_or_joined =
        std::any_of(m_status.begin(), m_status.end(), [](auto const& s) {
          return !s.second.isActive == (s.second.usedMem > 0);
        });
    auto const number_of_active_not_warming_pools =
        std::count_if(m_status.begin(), m_status.end(), [](auto const& s) {
          return is_active_not_warming(s.second);
        });

    ctx.m_pool_configs.clear();
    for (auto const& [id, pool] : m_status) {
      m_logger->info("{}: {} {}", id, pool.isActive ? "O" : "X", pool.usedMem);
      if (is_active_not_warming(pool)) {
        ctx.m_pool_configs[id].currentPartitioning =
            tenants_left_or_joined ? m_max / number_of_active_pools
                                   : m_status[id].usedMem;
        ctx.m_pool_configs[id].partitioning =
            ctx.m_pool_configs[id].currentPartitioning;
        ctx.m_pool_configs[id].MRC = std::move(m_mrcs[id]);
      }

      gsl_siman_params_t params = {
          .n_tries = 2000,
          .iters_fixed_T = 300,
          .step_size = 0.0, // not used
          .k = 1.0,
          .t_initial = 100,
          .mu_t = 1.003,
          .t_min = 0.1,
      };

      if (ctx.m_pool_configs.size() > 1) {
        gsl_siman_solve(r, &ctx, energy, step, distance, NULL, NULL, NULL, NULL,
                        (sizeof ctx), params);
      }
    }
  }

  void enforce() {
    // first add previous active pools as pools with 0 mem in partitioning
    std::vector<pid_t> targets{};
    std::unordered_map<pid_t, size_t> new_partitioning{};
    auto const tenants_left_or_joined =
        std::any_of(m_status.begin(), m_status.end(), [](auto const& s) {
          return !s.second.isActive == (s.second.usedMem > 0);
        });
    auto const number_of_active_pools =
        std::count_if(m_status.begin(), m_status.end(),
                      [](auto const& s) { return s.second.isActive; });
    for (auto const& [id, pool] : m_status) {
      if (is_active_not_warming(pool)) {
        new_partitioning[id] = ctx.m_pool_configs[id].partitioning;
      } else if (became_active(pool) ||
                 (pool.isActive && tenants_left_or_joined)) {
        new_partitioning[id] = m_max / number_of_active_pools;
      } else if (became_inactive(pool)) {
        new_partitioning[id] = 0;
      }
    }

    for (auto const& [id, _] : new_partitioning) {
      targets.push_back(id);
    }

    std::sort(targets.begin(), targets.end(),
              [&](auto const& a, auto const& b) {
                return (new_partitioning[a] -
                        static_cast<size_t>(m_status[a].usedMem)) <
                       (new_partitioning[b] -
                        static_cast<size_t>(m_status[b].usedMem));
              });

    for (auto const& id : targets) {
      m_logger->info("pool #{}: {} -> {}", id, m_status[id].usedMem,
                     new_partitioning[id]);
      cache->resize(id, new_partitioning[id]);
    }
  }

  static void step(const gsl_rng* r, void* xp, double step_size) {
    auto x = ((Context*)xp);
    auto const n = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    auto m = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    while (n == m) {
      m = gsl_rng_uniform_int(r, x->m_pool_configs.size());
    }
    auto receiver = x->m_pool_configs.begin();
    auto victim = x->m_pool_configs.begin();
    std::advance(receiver, n);
    std::advance(victim, m);

    double const delta =
        gsl_rng_uniform(r) *
        std::min({victim->second.partitioning,
                  (1 + x->delta_max) * receiver->second.currentPartitioning -
                      receiver->second.partitioning,
                  victim->second.partitioning -
                      (1 - x->delta_max) * victim->second.currentPartitioning});
    if (delta > 0) {
      victim->second.partitioning -= delta;
      receiver->second.partitioning += delta;
    }
  }

  /* now some functions to test in one dimension */
  static double energy(void* xp) {
    auto x = (Context*)xp;
    return std::accumulate(
        x->m_pool_configs.begin(), x->m_pool_configs.end(), 0.0,
        [](auto const& a, auto const& b) {
          return a + std::max(b.second.MRC(b.second.partitioning), 0.0);
        });
  }

  static double distance(void* xp, void* yp) {
    auto x = (Context*)xp;
    auto y = (Context*)yp;
    double result = 0;
    for (auto const& [id, pool] : x->m_pool_configs) {
      pow(pool.partitioning - y->m_pool_configs[id].partitioning, 2);
    }
    return sqrt(result);
  }

 public:
  void run() {
    collect();
    compute();
    enforce();
  }
  HitRatioMaximization(Cache* cache) : cache(cache) {
    try {
      m_logger = spdlog::basic_logger_st(
          "hitratiomaximization", "/tmp/hit_ratio_maximization.log", true);
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

  ~HitRatioMaximization() {
    m_logger->info("Destructing HitRatioMaximization");
    gsl_rng_free(r);
  }
};
} // namespace holpaca::control_algorithm
