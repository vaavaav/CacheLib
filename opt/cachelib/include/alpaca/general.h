#pragma once
#include <zmq.hpp>
#include <chrono>
#include <cstddef>
#include "alpaca/data_plane_stage/cache_interface.h"
#include "alpaca/data_plane_stage/substage/monitor/snapshot.h"

namespace alpaca {

    using Cache = std::shared_ptr<CacheInterface>;
    using time_t = std::chrono::seconds;
    using id_t = size_t;

    namespace config {
        zmq::context_t zmq_context {};
        char constexpr m2c_address[] { "inproc://m2c" };
        char constexpr signup_controller_address[] { "inproc://signup_controller"};

        namespace monitor {
            time_t constexpr periodicity { 1 };
            time_t constexpr m2c_timeout { 3 };
            size_t constexpr max_snapshots_len { 10 }; 
        }

        namespace controller {
            time_t constexpr c2t_timeout { 3 };
        }
    }
}