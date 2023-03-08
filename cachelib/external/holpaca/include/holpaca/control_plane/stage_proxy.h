#pragma once
#include <holpaca/config.h>
#include <holpaca/data_plane/stage_interface.h>
#include <thread>
#include <zmq.hpp>
#include <map>

namespace holpaca::control_plane {
    class StageProxy : public holpaca::data_plane::StageInterface {
        zmq::socket_t m_proxy;
        std::thread m_proxy_thread;
        public:
            StageProxy(zmq::context_t& zmq_context, char const* stage_socket_address, char const* log_file = {config::stage_proxy_log_file});
            void resize(id_t cache_id, size_t new_size); 
            std::map<id_t, size_t> getSizes() override; 
            std::map<id_t, size_t> getHits() override;
    };
}