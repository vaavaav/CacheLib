#pragma once
#include <holpaca/config.h>
#include <holpaca/data_plane/stage_interface.h>
#include <thread>
#include <zmq.hpp>

using namespace holpaca::config::control_plane;

namespace holpaca::control_plane {
    class StageProxy : public holpaca::data_plane::StageInterface {
        zmq::socket_t m_proxy;
        std::thread m_proxy_thread;
        public:
            StageProxy(zmq::context_t& zmq_context, char const* stage_socket_address, char const* log_file = {stage_proxy_log_file});
            void resizeCache(id_t cache_id, size_t const new_size) override final;
            std::vector<holpaca::snapshot_t> getFullSnapshot() override final;
    };
}