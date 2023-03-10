#include <holpaca/data_plane/autonomous_stage.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>

namespace holpaca::data_plane {
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
        SPDLOG_LOGGER_INFO(m_logger, "Autonomous Stage created");
    } 
}