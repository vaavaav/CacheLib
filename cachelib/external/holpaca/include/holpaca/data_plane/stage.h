#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage_interface.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <thread>
#include <zmq.hpp>

using namespace holpaca::config::data_plane;

namespace holpaca::data_plane {
    class Stage : public StageInterface {
        zmq::context_t m_zmq_context;
        zmq::socket_t m_socket;
        std::atomic_bool m_stop { false };
        std::thread m_stage_thread;
        void controllerRegistry(char const* stage_socket_address, char const* registry_address = {config::control_plane::controller_registry_socket_address});
        void sendSnapshot();
        void run();

       public:
            Stage(
                char const* log_file = {stage_log_file},
                char const* socket_address = {stage_socket_address},
                holpaca::time_t const timeout = {stage_timeout});
            ~Stage();
    };
}