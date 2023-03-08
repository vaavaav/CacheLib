#include <holpaca/config.h>
#include <holpaca/data_plane/detached_stage.h>
#include <holpaca/utils/message.h>
#include <holpaca/utils/zmq_extra.h>
#include <iostream>
#include <spdlog/sinks/basic_file_sink.h>

namespace holpaca::data_plane {

    DetachedStage::DetachedStage(char const* socket_address,
            holpaca::time_t const timeout,
            char const* log_file
    ) {
        spdlog::flush_every(5s);
        try {
            logger = spdlog::basic_logger_mt("detached_stage", log_file);
            logger->set_level(spdlog::level::debug);
            logger->flush_on(spdlog::level::err);
        } catch (const spdlog::spdlog_ex &ex) {
            std::cerr << "Log init failed: " << ex.what() << std::endl;
            abort();
        }
        SPDLOG_LOGGER_INFO(logger, "Initializing Stage");
        m_zmq_context = zmq::context_t();
        SPDLOG_LOGGER_INFO(logger, "Created ZMQ context");
        m_socket = zmq::socket_t(m_zmq_context, zmq::socket_type::rep);
        SPDLOG_LOGGER_INFO(logger, "Created socket of type: {}", zmq::to_string(m_socket.get(zmq::sockopt::socket_type)));
        try {
            m_socket.bind(socket_address);
        } catch (zmq::error_t e) {
            SPDLOG_LOGGER_ERROR(logger, 
                "Could not bind skeleton socket to '{}': {}",
                socket_address, + e.what()
            );
        };
        controllerRegistry(socket_address);
        run();
    }

    DetachedStage::~DetachedStage() {
        SPDLOG_LOGGER_INFO(logger, "Destructing Stage");
        m_stop.store(true);
        m_stage_thread.join();
    }

    void DetachedStage::sendSnapshot() {
        SPDLOG_LOGGER_INFO(logger, "Sending snapshots!");
    }

    void DetachedStage::run() {
        SPDLOG_LOGGER_INFO(logger, "Run!");
        m_stage_thread = std::thread { [this]() {
            zmq::message_t msg {};
            while(!m_stop.load()) {
                static_cast<void>(m_socket.recv(msg, zmq::recv_flags::none));
                SPDLOG_LOGGER_INFO(logger, "Received message!");
                Message::request_type rt = Message::unserialize(msg);
                switch(rt) {
                    case Message::request_type::snapshot: 
                        sendSnapshot(); break;
                    default: continue;
                }
            };
        }};

    }

    void DetachedStage::controllerRegistry(char const* stage_socket_address, char const* registry_address) {
        SPDLOG_LOGGER_INFO(logger, "Beginning registry!");
        zmq::socket_t m_temp_socket {m_zmq_context, zmq::socket_type::req };
        SPDLOG_LOGGER_INFO(logger, "Created tmeporary socket of type {}", zmq::to_string(m_temp_socket.get(zmq::sockopt::socket_type)));
        try {
            m_temp_socket.connect(registry_address);
        } catch (zmq::error_t e) {
            SPDLOG_LOGGER_ERROR(logger, 
                "Could not connect to registry socket '{}': {}", registry_address, e.what()
            );
            abort();
        }
        SPDLOG_LOGGER_INFO(logger, "Registry socket binded to '{}'", registry_address);
        zmq::const_buffer msg = zmq::buffer (std::string(stage_socket_address));
        auto result = m_temp_socket.send(msg);
        if(!result.has_value()) {
            SPDLOG_LOGGER_ERROR(logger, 
                "Could not send message to controller at '{}'", 
                registry_address
            );
            abort();
        }
    }
}