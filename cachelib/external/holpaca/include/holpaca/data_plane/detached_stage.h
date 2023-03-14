#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <memory>
#include <thread>
#include <zmq.hpp>

namespace holpaca::data_plane {
    class DetachedStage : public Stage {
        zmq::context_t m_zmq_context;
        zmq::socket_t m_socket;
        std::atomic_bool m_stop { false };
        std::thread m_stage_thread;
        std::shared_ptr<spdlog::logger> logger;
        void controllerRegistry(char const* stage_socket_address, char const* registry_address = {holpaca::control_plane::config::controller_registry_socket_address});
        void sendSnapshot();
        void run();

       public:
            DetachedStage(char const* socket_address = {config::stage_address},
                holpaca::time_t const timeout = {config::stage_timeout},
                char const* log_file = { config::stage_log_file }
            );
            ~DetachedStage();
    };
}
