#pragma once
#include <zmq.hpp>
#include <vector>
#include "snapshot.h"

namespace holpaca {
    class Message {
        public:
            enum request_type : int {
                resize,
                snapshot,
            };
            static request_type unserialize(zmq::message_t& msg) {
                const Message::request_type* request_type = (msg.data<Message::request_type>());
                return *request_type;
            }
    };
}
