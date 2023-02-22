#pragma once 
#include "alpaca/general.h"

namespace alpaca::data_plane_stage::substage {
    class Tuner {
        public:
            Tuner() = delete;
            Tuner(std::string& ip, Cache& cache_instance, std::string& const controller_address);
            ~Tuner();

        private:
            Cache m_cache_instance;
            zmq::socket_t m_c2t_channel;
    };
}