#pragma once
#include <holpaca/common/cache.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <unordered_set>
extern "C" {
#include <gsl/gsl_siman.h>
};

using namespace holpaca::common;

namespace holpaca::control_algorithm {

#define OBJECT_SIZE 1000
#define THRESHOLD 0.05

struct Context {
  double max{0};
  std::vector<pid_t> pids{};
  std::unordered_map<pid_t, std::vector<std::pair<uint32_t, double>>> mrcs{};
  std::unordered_map<pid_t, double> sizes{};
  std::unordered_map<pid_t, double> bestSizes{};

  double estimate_mr(pid_t i) {
    auto const& mrci = mrcs[i];
    double const& size = sizes[i];
    if (mrci.size() == 0) {
      return bestSizes[i] / size;
    }
    if (size < mrci.front().first) {
      return mrci.front().second * mrci.front().first / size;
    }
    for (std::size_t i = 0; (i + 1) < mrci.size(); i++) {
      if (mrci[i].first <= size && size <= mrci[i + 1].first) {
        double const d = mrci[i + 1].first - mrci[i].first;
        double const r =
            mrci[i].second * (1 - (size - mrci[i].first) / d) +
            mrci[i + 1].second * (1 - (mrci[i + 1].first - size) / d);
        return r;
      }
    }
    return mrci.back().second * mrci.back().first / size;
  }
};

class MissRateMin {
  MissRateMin() = delete;
  Cache* cache = NULL;
  std::shared_ptr<spdlog::logger> m_logger;
  std::ofstream mrc_log;

 public:
  std::unordered_set<pid_t> inactive{};
  std::vector<pid_t> active{};
  const gsl_rng_type* T;
  gsl_rng* r;
  Context ctx{};
  double bestE{1000000};

  void collect() {
    if (cache == NULL) {
      return;
    }
    double max = cache->size() / static_cast<double>(OBJECT_SIZE);
    ctx.max = max;
    ctx.pids.clear();
    ctx.mrcs.clear();
    ctx.sizes.clear();

    m_logger->info("Collecting cache status");

    auto statuses = cache->getStatus();
    int nactive =
        std::count_if(statuses.begin(), statuses.end(),
                      [](auto const& s) { return s.second.isActive; });

    mrc_log << "{";
    for (auto const& [pid, s] : statuses) {
      if (s.isActive) {
        mrc_log << "{" << pid << ",{";
        ctx.pids.push_back(pid);
        std::vector<uint32_t> keys;
        keys.reserve(s.mrc.size());
        for (auto it = s.mrc.begin(); it != s.mrc.end(); ++it) {
          keys.push_back(it->first);
        }
        std::sort(keys.begin(), keys.end());
        std::vector<std::pair<uint32_t, double>> mrc;
        mrc.reserve(keys.size());
        for (auto const& k : keys) {
          auto const v = s.mrc.at(k);
          m_logger->debug("{}:{},{}", pid, k, v);
          mrc.push_back({k, v});
          mrc_log << "{" << k << "," << v << "},";
        }
        ctx.mrcs[pid] = mrc;
        mrc_log << "}},";
        if (inactive.find(pid) != inactive.end()) {
          inactive.erase(pid);
        }
      } else {
        // preemtive enforce
        if (inactive.find(pid) == inactive.end()) {
            cache->resize(pid, 0);
            inactive.insert(pid);
        }
      }
    }
    mrc_log << "}" << std::endl;
    m_logger->debug("max={}", ctx.max);
    bool changed = false;
    for (auto const& pid : ctx.pids) {
      if (std::find(active.begin(), active.end(), pid) == active.end()) {
        changed = true;
        break;
      }
    }
    for (auto const& pid : active) {
      if (std::find(ctx.pids.begin(), ctx.pids.end(), pid) == ctx.pids.end()) {
        changed = true;
        break;
      }
    }

    if (changed) {
      active = ctx.pids;
      ctx.bestSizes.clear();
      auto size = max / nactive;
      for (auto const& pid : ctx.pids) {
        // preemptive enforce
        cache->resize(pid, size* OBJECT_SIZE);
        ctx.bestSizes[pid] = size;
      }
      ctx.sizes = ctx.bestSizes;
      bestE = E1(&ctx);
    } else {
      ctx.sizes = ctx.bestSizes;
    }
  }

  void compute() {
    if (cache == NULL || ctx.pids.size() == 0) {
      return;
    }
    m_logger->debug("computing for {} pools", ctx.pids.size());
    gsl_siman_params_t params = {
        .n_tries = 20,
        .iters_fixed_T = 100,
        .step_size = 50.0,
        .k = 1.0,
        .t_initial = 5000.0,
        .mu_t = 1.003,
        .t_min = 1.0e-2,
    };

    gsl_siman_solve(r, &ctx, E1, S1, M1, NULL, NULL, NULL, NULL, (sizeof ctx),
                    params);
    auto e = E1(&ctx);

    if (e < bestE) {
      bestE = e;
      ctx.bestSizes = ctx.sizes;
    }
  }
  void enforce() {
    if (cache == NULL || ctx.pids.size() == 0) {
      return;
    }
    double mr{0};
    for (auto& [id, size] : ctx.bestSizes) {
      m_logger->debug("pid={} new size={}", id, size);
      cache->resize(id, size * OBJECT_SIZE);
      mr += ctx.estimate_mr(id);
    }
    m_logger->debug("Energy={}", E1(&ctx));
  }

  static void S1(const gsl_rng* r, void* xp, double step_size) {
    auto x = ((Context*)xp);
    for (auto const& id : x->pids) {
      double u = gsl_rng_uniform(r);
      x->sizes[id] += (u * 2 - 1) * step_size;
    }
  }
  /* now some functions to test in one dimension */
  static double E1(void* xp) {
    auto x = *(Context*)xp;
    double tsize{0};
    double mr{0};
    for (auto const& [id, size] : x.sizes) {
      if (size <= 0 || size > x.max) {
        return 100000000;
      }
      tsize += size;
    }
    if (tsize > x.max) {
      return 100000000;
    }
    for (auto const& id : x.pids) {
      mr += x.estimate_mr(id);
    }
    return mr;
  }

  static double M1(void* xp, void* yp) {
    auto x = *((Context*)xp);
    auto y = *((Context*)yp);
    if (xp == NULL || yp == NULL) {
      return 0;
    }
    double s{0};
    for (auto const& id : x.pids) {
      s += fabs(x.sizes[id] - y.sizes[id]);
    }

    return s;
  }

 public:
  void run() {
    collect();
    compute();
    enforce();
  }
  MissRateMin(Cache* cache) : cache(cache) {
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
    T = gsl_rng_default;
    r = gsl_rng_alloc(T);
    mrc_log.open("/tmp/mrc.log");
  }
  ~MissRateMin() {
    m_logger->info("Destructing MissRateMin");
    gsl_rng_free(r);
    mrc_log.close();
  }
};
} // namespace holpaca::control_algorithm