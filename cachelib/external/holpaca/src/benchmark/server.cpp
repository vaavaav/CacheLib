#include <netinet/in.h>
#include <iostream>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 8080

int main(int argc, char const* argv[]) {
    int server_fd;
    int new_socket;
    int valread;
    struct sockaddr_in address;

    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[1024] = { 0 };

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        std::cerr << "socket failed" << std::endl;
        abort();
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        std::cerr << "setsockopt" << std::endl;
        abort();
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        std::cerr << "bind failed" << std::endl;
        abort();
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "listen" << std::endl;
        abort();
    }

    if ((new_socket = accept (server_fd, (struct sockaddr*) &address, (socklen_t*) &addrlen)) < 0) {
        std::cerr << "accept" << std::endl;
        abort();
    }

    valread = read(new_socket, buffer, 1024);
    printf("%s\n", buffer);

    close(new_socket);
    shutdown(server_fd, SHUT_RDWR);
}
