#pragma once
#include <holpaca/data_plane/stage.h>
#include <spdlog/spdlog.h>
#include <holpaca/control_algorithm/control_algorithm.h>
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
            ~AutonomousStage() {
                for(auto& ca : m_control_algorithms) {
                    ca.reset();
                }
            }
            template<typename T>
            void addControlAlgorithm(std::chrono::milliseconds const periodicity) {
                m_control_algorithms.push_back(
                    std::make_shared<T>(m_cache, periodicity)
                );
                SPDLOG_LOGGER_INFO(m_logger, "Added Control Algorithm <{}>", typeid(T).name());
            };
    };
}
