#include <holpaca/control_plane/controller.h>
#include <holpaca/utils/zmq_extra.h>

namespace holpaca::control_plane {
  Controller::Controller(char const* log_file, char const* registry_socket_address) {
    //plog::init(plog::debug, log_file);
    //PLOGI << "Initializing Controller";
    m_zmq_context = zmq::context_t();
    m_registry_socket = zmq::socket_t(m_zmq_context, zmq::socket_type::rep);
    //PLOGI << "Created registry socket (of type: "
    //      << zmq::to_string(m_registry_socket.get(zmq::sockopt::socket_type))
    //      << ")";
    try {
      m_registry_socket.bind(registry_socket_address);
    } catch (zmq::error_t e) {
     // PLOGE << "Could not bind registry socket to '" << registry_socket_address
     //       << "': " << e.what();
      abort();
    }
    //PLOGI << "Registry socket binded to '" << 
    //m_registry_socket.get(zmq::sockopt::bindtodevice)  << "'";
    run();
  }

  void Controller::run() {
    //PLOGI << "Running controller";
    zmq::message_t msg;
    while(true) {
      m_registry_socket.recv(msg, zmq::recv_flags::none);
     // PLOGI << "Received message: " << msg.to_string();
      msg = {"pong!", 5};
      m_registry_socket.send(msg, zmq::send_flags::none);
    }
  }
}

int main() {
  holpaca::control_plane::Controller c {};
  return 0;
}