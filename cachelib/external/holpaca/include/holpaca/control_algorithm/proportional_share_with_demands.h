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

struct Context {
  struct PoolCtx {
    double demand;
    double partitioning;
  } std::unordered_map<pid_t, PoolCtx> m_pool_ctxs{};
};

class ProportionalShareWithDemands {
  Cache* cache = NULL;
  std::shared_ptr<spdlog::logger> m_logger;
  std::unordered_map<pid_t, double> m_current_partitioning{};
  std::unordered_map<pid_t, double> demands{};
  Status m_status{};
  double m_max{0};
  const gsl_rng_type* T;
  gsl_rng* r;
  Context ctx{};

 public:
  void collect() {
    if (cache == NULL) {
      m_logger->warn("Cache is NULL. Skipping collect.")
    }

    m_logger->info("Collecting cache status");
    m_max = cache->size();
    m_status = cache->getStatus();
  }

  void compute() {
    m_logger->info("Computing new partitioning");
    int const number_of_active_pools =
        std::count_if(statuses.begin(), statuses.end(),
                      [](auto const& s) { return s.second.isActive; });
    auto const default_size = max / number_of_active_pools;
    ctx.m_pool_ctxs.clear();
    for (auto const& [id, pool] : statuses) {
      if (pool.isActive) {
        m_current_partitioning[id] = pool.usedMem;
        ctx.m_pool_ctxs[id].partitioning = default_size;
        ctx.m_pool_ctxs[id].demand = ctx.demands[id];
      }
    }
    if (number_of_active_pools > 1) {
      gsl_siman_params_t params = {
          .n_tries = 1000,
          .iters_fixed_T = 300,
          .step_size = 0.0, // not used
          .k = 1.0,
          .t_initial = 100,
          .mu_t = 1.003,
          .t_min = 0.1,
      };
      gsl_simal_solve(r, &ctx, energy, step, distance, NULL, NULL, NULL, NULL,
                      (sizeof ctx), params);
    }
  }

  void enforce() {
    // first add previous active pools as pools with 0 mem in partitioning
    std::vector<pid_t> targets{};
    std::unordered_map<pid_t, double> new_partitioning{m_current_partitioning};
    for (auto const& [id, pool] : m_status) {
      if (!pool.isActive &&
          m_current_partitioning.find(id) != m_current_partitioning.end()) {
        new_partitioning[id] = 0;
      } else if (auto pctx = ctx.m_pool_ctxs.find(id);
                 pctx != ctx.m_pool_ctxs.end()) {
        new_partitioning[id] = pctx->second.partitioning;
      }
    }
    for (auto const& [id, _] : new_partitioning) {
      targets.push_back(id);
    }

    std::sort(pools.begin(), pools.end(), [&](auto const& a, auto const& b) {
      return (new_partitioning[a] - m_current_partitioning[a]) <
             (new_partitioning[b] - m_current_partitioning[b]);
    });

    for (auto const& id : targets) {
      cache->resize(id, new_partitioning[id]);
      if (!m_status[id].isActive &&
          m_current_partitioning.find(id) != m_current_partitioning.end()) {
        m_current_partitioning.erase(id);
      }
    }
  }

  static void step(const gsl_rng* r, void* xp, double step_size) {
    auto x = ((Context*)xp);
    auto const n = gsl_rng_uniform_int(r, x->m_pool_ctxs.size());
    auto m = gsl_rng_uniform_int(r, x->m_pool_ctxs.size());
    while (n == m) {
      m = gsl_rng_uniform_int(r, x->partitioning.size());
    }
    auto receiver = x->m_pool_ctxs.begin();
    auto victim = x->m_pool_ctxs.begin();
    std::advance(receiver, n);
    std::advance(victim, m);

    double const delta =
        gsl_rng_uniform(r) *
        std::min(victim->second.partitioning,
                 receiver->second.demand - receiver->second.partitioning);
    if (delta > 0) {
      receiver->second.m_size += delta;
      victim->second.m_size -= delta;
    }
  }

  /* now some functions to test in one dimension */
  static double energy(void* xp) {
    return std::accumulate(
        x->m_pool_configs.begin(), x->m_pool_configs.end(), 0.0,
        [](auto const& a, auto const& b) {
          return a + abs(b.second.demand - b.second.partitioning);
        });
  }

  static double distance(void* xp, void* yp) {
    auto x = (Context*)xp;
    auto y = (Context*)yp;
    double result = 0;
    for (auto const& [id, pool] : x->m_pool_ctxs) {
      pow(pool.partitioning - y->m_pool_ctxs[id].partitioning, 2);
    }
    return sqrt(result);
  }

 public:
  void run() {
    collect();
    compute();
    enforce();
  }
  ProportionalShareWithDemands(Cache* cache,
                               std::unordered_map<pid_t, double> demands)
      : cache(cache) {
    try {
      m_logger = spdlog::rotating_logger_st(
          "proportional_share_with_demands",
          "/tmp/proportional_share_with_demands.log", 1024 * 1024, 0, false);
    } catch (const spdlog::spdlog_ex& ex) {
      std::cerr << "Log init failed: " << ex.what() << std::endl;
      abort();
    }
    m_logger->flush_on(spdlog::level::trace);
    m_logger->set_level(spdlog::level::debug);
    m_logger->info("Initialization");
    for (auto const& [id, demand] : demands) {
      this->demands[id] = demand;
    }
    gsl_rng_env_setup();
    r = gsl_rng_alloc(gsl_rng_default);
  }

  ~MissRateMin() {
    m_logger->info("Destructing ProportionalShareWithDemands");
    gsl_rng_free(r);
  }
};
} // namespace holpaca::control_algorithm
