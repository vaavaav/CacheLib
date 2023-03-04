#pragma once
#include <zmq.hpp>
#include <string>

namespace zmq {
    inline auto constexpr to_string(zmq::socket_type socket_type) {
        switch(socket_type) {
            case socket_type::dealer : return "DEALER";
            case socket_type::pair   : return "PAIR";
            case socket_type::pub    : return "PUB";
            case socket_type::pull   : return "PULL";
            case socket_type::push   : return "PUSH";
            case socket_type::rep    : return "REP";
            case socket_type::req    : return "REQ";
            case socket_type::router : return "ROUTER";
            case socket_type::stream : return "STREAM";
            case socket_type::sub    : return "SUB";
            case socket_type::xpub   : return "XPUB";
            case socket_type::xsub   : return "XSUB";
            default: return "(?)";
        };
    }
}