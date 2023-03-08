#pragma once
#include <holpaca/config.h>
#include <atomic>
#include <thread>
#include <chrono>

namespace holpaca::control_algorithm {
    class ControlAlgorithm {
        std::atomic_bool m_stop {false};
        std::thread m_thread;

        protected:
            virtual void collect() = 0;
            virtual void compute() = 0;
            virtual void enforce() = 0;

        public:
            ControlAlgorithm(std::chrono::seconds const periodicity) { 
                m_thread = std::thread {
                    [this, periodicity]() {
                        while(!m_stop) {
                            collect();compute();enforce();
                            std::this_thread::sleep_for(periodicity);
                        }
                    }
                };
            }
            ~ControlAlgorithm() {
                m_stop = true;
                m_thread.join();
            }
    };
}