#include <holpaca/data_plane/autonomous_stage.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <holpaca/control_algorithm/naive_control_algorithm.h>

namespace holpaca{
    namespace data_plane {
        AutonomousStage::AutonomousStage(
            Cache* cache,
            char const* log_file
        ) : Stage(cache) {
            try {
                m_logger = spdlog::basic_logger_mt("stage", config::stage_log_file);
            } catch (const spdlog::spdlog_ex &ex) {
                std::cerr << "Log init failed: " << ex.what() << std::endl;
                abort();
            }
            m_logger->flush_on(spdlog::level::debug);
            m_logger->set_level(spdlog::level::trace);
            m_control_algorithms.push_back(
                std::make_shared<NaiveControlAlgorithm>(
                    m_cache,
                    100ms
                )
            );
            m_logger->info("Autonomous stage created");
        } 
    }
}
