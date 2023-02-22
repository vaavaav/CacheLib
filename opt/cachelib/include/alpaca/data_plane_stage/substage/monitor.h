#pragma once
#include "alpaca/general.h"
#include <zmq.hpp>
#include <atomic>
#include <thread>

using atomic_time_t = std::atomic<alpaca::time_t>;
using namespace alpaca::data_plane_stage::substage::monitor;

namespace alpaca::data_plane_stage::substage {
    class Monitor {
        public:
            Monitor& setMonitoringPeriodicity(time_t& periodicity);
            Monitor() = delete;
            Monitor(id_t id, Cache& cache_instance, std::string& const m2c_address);
            id_t const id;
            size_t getMetricsCollectionLen() const;
            std::string const m2c_address { alpaca::config::m2c_address };
            time_t getMonitoringPeriodicity() const;
            time_t setM2CTimeout() const;
            ~Monitor();

        private:
            Cache m_cache_instance;
            Snapshot m_snapshots[ alpaca::config::monitor::max_snapshots_len ];
            atomic_time_t m_periodicity { alpaca::config::monitor::periodicity }; 
            size_t m_snapshots_len { 0 };
            std::thread m_monitor_thread;
            zmq::socket_t m_m2c_channel;
    };
}