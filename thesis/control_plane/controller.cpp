#include <zmq.hpp>
#include "../constants.hpp"
#include <iostream>

static zmq::context_t singleton_context;

int main() {
    zmq::socket_t metrics_sock (singleton_context, zmq::socket_type::pull);
    metrics_sock.bind("ipc://" + metrics_socket_address);

    zmq::message_t metrics_msg;
    while(true) {
        metrics_sock.recv(&metrics_msg);
        std::cout << metrics_msg.to_string() << std::endl;
    }
    
    metrics_sock.close();
}