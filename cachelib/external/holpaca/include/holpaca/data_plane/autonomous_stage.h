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
                std::chrono::milliseconds periodicity,
                char const* log_file = { config::stage_log_file }
            ); 

            ~AutonomousStage() {
                m_logger->info("Destructing Stage");
                for(auto& ca : m_control_algorithms) {
                    m_logger->debug("Destructing control algorithm");
                    ca.reset();
                }
            }
            template<typename T>
            void addControlAlgorithm(std::chrono::milliseconds const periodicity) {
                m_control_algorithms.push_back(
                    std::make_shared<T>(m_cache, periodicity)
                );
                //m_logger->info("Added control algorithm <{}>", typeid(T).name());
            };
    };
}
