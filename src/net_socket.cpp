#include "net_socket.h"

#include <iostream>
#include <string>
#include <cstdint>
#include <system_error>

namespace {

std::string GetSocketErrorString() {
    static const std::error_category& category = std::system_category();
    return category.message(GetLastSocketError());
}

}  // namespace

socket_t CreateListener(const char* ip, uint16_t port) {
    const socket_t server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == kInvalidSocket) {
        std::cerr << "socket() failed: " << GetSocketErrorString() << "\n";
        return kInvalidSocket;
    }
    constexpr int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR,
        reinterpret_cast<const char*>(&opt), sizeof(opt)) < 0) {
        std::cerr << "setsockopt() failed: " << GetSocketErrorString() << "\n";
        CloseSocket(server_fd);
        return kInvalidSocket;
    }
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address\n";
        CloseSocket(server_fd);
        return kInvalidSocket;
    }
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "bind() failed on port " << port << ": " << GetSocketErrorString() << "\n";
        CloseSocket(server_fd);
        return kInvalidSocket;
    }
    if (listen(server_fd, SOMAXCONN) < 0) {
        std::cerr << "listen() failed: " << GetSocketErrorString() << "\n";
        CloseSocket(server_fd);
        return kInvalidSocket;
    }
    return server_fd;
}