#include <holpaca/control_plane/stage_proxy.h>
#include <holpaca/utils/zmq_extra.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/spdlog.h>
#include <zmq.hpp>

namespace holpaca::control_plane {

    StageProxy::StageProxy(zmq::context_t& zmq_context, char const* stage_socket_address, char const* log_file) {

        //plog::init(plog::debug, log_file);
        //PLOGI << "Initializing Stage Proxy";
        m_proxy = zmq::socket_t(zmq_context, zmq::socket_type::req);
       // PLOGI << "Created proxy socket (of type: " 
       //       << zmq::to_string(m_proxy.get(zmq::sockopt::socket_type))
       //       << ")";
        try {
            m_proxy.connect(stage_socket_address);
        } catch (zmq::error_t e) {
        //    PLOGE << "Proxy could not connect to '"
        //          << stage_socket_address
        //          << "': " << e.what();
            abort();
        }
    }


    std::vector<snapshot_t> StageProxy::getFullSnapshot() {
        //PLOGI << "getFullSnapshot()";

        return {};
    }

    void StageProxy::resizeCache(id_t cache_id, size_t const new_size) {
        //PLOGI << "resizeCache( " << cache_id << ", "  << new_size << " )";
    }
}