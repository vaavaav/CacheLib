#pragma once
#include <atomic>
#include <holpaca/config.h>
#include <holpaca/data_plane/stage_interface.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <string>
#include <unordered_map>
#include <zmq.hpp>

using namespace holpaca::config::control_plane;
using namespace holpaca::data_plane;

namespace holpaca::control_plane {
    class Controller {
      std::unordered_map<id_t, std::unique_ptr<StageInterface>> m_stages;
      zmq::socket_t m_registry_socket;
      zmq::context_t m_zmq_context;

      void registerStage(std::unique_ptr<StageInterface> stage);
      void run();

      public:
        Controller(char const* log_file = {controller_log_file}, char const* registry_socket_address = { controller_registry_socket_address});
    };
}