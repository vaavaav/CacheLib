#include <zmq.hpp>
#include "../constants.hpp"
#include <iostream>
#include <thread>
#include <fstream>
#include <iostream>

static zmq::context_t singleton_context;

void metricsReceiver() {
    std::ofstream log (log_file);
    zmq::socket_t metrics_sock (singleton_context, zmq::socket_type::pull);
    metrics_sock.bind("ipc://" + metrics_socket_address);

    zmq::message_t metrics_msg;
    while(true) {
        metrics_sock.recv(&metrics_msg);
        log << metrics_msg.to_string() << std::endl;
    }
    
    metrics_sock.close();
}


int main() {
    std::thread metricsReceiverT (metricsReceiver);

    int number_of_instances = sizeof instances / sizeof instances[0];
    zmq::socket_t poolSockets[number_of_instances];

    for(int i = 0; i < number_of_instances; i++) {
        poolSockets[i] = zmq::socket_t (singleton_context, zmq::socket_type::push); 
        poolSockets[i].connect("ipc://" + instances[i]);
    }

    int new_size;
    int pool_id;
    while(true) {
        std::cin >> pool_id >> new_size;
        zmq::message_t message (std::to_string(new_size));
        poolSockets[pool_id].send(message);
    }
}