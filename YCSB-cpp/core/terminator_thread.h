#pragma once
#include "core_workload.h"
#include <thread>
#include <future>

namespace ycsbc {
    inline void TerminatorThread(std::chrono::seconds max_execution_time, ycsbc::CoreWorkload *wl, std::vector<std::future<int>> *threads) {
        std::this_thread::sleep_for(max_execution_time);
        wl->request_stop();
        for (auto& t : *threads) {
            t.wait();
        }
    }
}