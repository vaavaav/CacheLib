#pragma once
#include <holpaca/config.h>
#include <iostream>
#include <atomic>
#include <memory>
#include <thread>
#include <chrono>
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_DEBUG
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace holpaca::control_algorithm {
    class ControlAlgorithm {
        std::atomic_bool m_stop {false};
        std::thread m_thread;

        protected:
            virtual void collect() = 0;
            virtual void compute() = 0;
            virtual void enforce() = 0;
            std::shared_ptr<spdlog::logger> m_logger;

        public:
            ControlAlgorithm(std::chrono::milliseconds const periodicity) { 
                try {
                    m_logger = spdlog::basic_logger_mt("ca", "/tmp/ca.log");
                } catch (const spdlog::spdlog_ex& ex) {
                    std::cerr << "Log init failed: " << ex.what() << std::endl;
                    abort();
                }
                m_logger->flush_on(spdlog::level::trace);
                m_logger->set_level(spdlog::level::debug);
                    m_thread = std::thread {
                    try {
                            [this, periodicity]() {
                                while(!m_stop) {
                                    collect();compute();enforce();
                                    std::this_thread::sleep_for(periodicity);
                                }
                            }
                        } catch (...) {
                            if(!m_stop) {
                                std::cerr << "Unexpected error" << std::endl;
                            }
                        };
                    };
            }
            ~ControlAlgorithm() {
                m_stop = true;
                m_thread.join();
            }
    };
}
