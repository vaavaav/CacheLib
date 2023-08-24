#pragma once
#include <holpaca/data_plane/stage.h>
#include <spdlog/spdlog.h>
#include <memory>
#include <thread>

using namespace holpaca::common;

namespace holpaca::data_plane {
    class AutonomousStage : public Stage {
        std::shared_ptr<spdlog::logger> m_logger;
        std::vector<std::thread> m_control_algorithms;

        public: 
            AutonomousStage(
                Cache* cache,
                std::chrono::milliseconds periodicity,
                char const* log_file = { config::stage_log_file }
            ); 

            ~AutonomousStage() {
                m_logger->info("Destructing Stage");
            }
            template<typename T>
            void addControlAlgorithm(std::chrono::milliseconds const periodicity) {
                m_control_algorithms.push_back(
                    std::thread { [&]() {
                        T t (cache);
                        while(true){
                            t.run();
                            std::this_thread::sleep_for(periodicity);
                        }
                    }}
                );
                //m_logger->info("Added control algorithm <{}>", typeid(T).name());
            };
    };
}
