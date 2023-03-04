#include <holpaca/config.h>
#include <holpaca/data_plane/stage.h>
#include <holpaca/utils/message.h>
#include <holpaca/utils/zmq_extra.h>
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>

namespace holpaca::data_plane {

    Stage::Stage( char const* log_file,
            char const* socket_address,
            holpaca::time_t const timeout) {
        try {
            spdlog::basic_logger_mt("stage_logger", "/tmp/stage-log.txt");
        } catch (const spdlog::spdlog_ex &ex) {
            std::cerr << "Log init failed: " << ex.what() << std::endl;
        }
        spdlog::info("Initializing Stage");
        m_zmq_context = zmq::context_t();
        spdlog::info("Created ZMQ context");
        m_socket = zmq::socket_t(m_zmq_context, zmq::socket_type::rep);
        spdlog::info("Created socket of type: {}", zmq::to_string(m_socket.get(zmq::sockopt::socket_type)));
        try {
            m_socket.bind(socket_address);
        } catch (zmq::error_t e) {
            spdlog::error("Could not bind skeleton socket to '{}': {}",
                socket_address, + e.what()
            );
        };
        controllerRegistry(stage_socket_address);
        run();
    }

    Stage::~Stage() {
        spdlog::info("Destructing Stage");
        m_stop.store(true);
        m_stage_thread.join();
    }

    void Stage::sendSnapshot() {
        spdlog::info("Sending snapshots!");
        m_socket.send(zmq::message_t(getFullSnapshot()), zmq::send_flags::none);
    }

    void Stage::run() {
        spdlog::info("Run!");
        m_stage_thread = std::thread { [this]() {
            zmq::message_t msg {};
            while(!m_stop.load()) {
                m_socket.recv(msg, zmq::recv_flags::none);
                spdlog::info("Received message");
                Message::request_type rt = Message::unserialize(msg);
                switch(rt) {
                    case Message::request_type::snapshot: 
                        sendSnapshot(); break;
                    default: continue;
                }
            };
        }};

    }

    void Stage::controllerRegistry(char const* stage_socket_address, char const* registry_address) {
        spdlog::info ("Beginning registry");
        zmq::socket_t m_temp_socket {m_zmq_context, zmq::socket_type::req };
        spdlog::info("Created temporary socket of type {}", 
            zmq::to_string(m_temp_socket.get(zmq::sockopt::socket_type)));
        try {
            m_temp_socket.connect(registry_address);
        } catch (zmq::error_t e) {
            spdlog::error("Could not binf skeleton socket to '{}': {}",
                registry_address, e.what()
            );
            abort();
        }
        spdlog::info("Registry socket binded to '{}'", m_temp_socket.get(zmq::sockopt::bindtodevice));
        zmq::const_buffer msg = zmq::buffer (std::string(stage_socket_address));
        auto result = m_temp_socket.send(msg);
        if(!result.has_value()) {
            spdlog::error("Could not send message to controller at '{}'", registry_address);
            abort();
        }
    }
}