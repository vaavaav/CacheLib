#pragma once
#include <holpaca/data_plane/stage.h>
#include <holpaca/control_algorithm/control_algorithm.h>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <memory>

using namespace holpaca::control_algorithm;

namespace holpaca::data_plane {
    class AutonomousStage : public Stage {
        std::shared_ptr<spdlog::logger> m_logger;
        std::vector<std::shared_ptr<ControlAlgorithm>> m_control_algorithms;
        public: 
            AutonomousStage(
                Cache* cache,
                char const* log_file = { config::stage_log_file }
            ); 
//            template<typename T>
//            void addControlAlgorithm() {
//                m_control_algorithms.push_back(
//                    std::make_shared<T>(m_cache)
//                );
//            }
            template<typename T, typename... Args>
            void addControlAlgorithm(Args&& ...args) {
                m_control_algorithms.push_back(
                    std::make_shared<T>(m_cache, std::forward(args...))
                );
                SPDLOG_LOGGER_INFO(m_logger, "Added Control Algorithm <{}>", typeid(T).name());
            };
    };
}