#include <future>
#include <iostream>
#include <string>
#include <string_view>
#include <vector>

#include "dashboard_render.h"
#include "http_probe.h"
#include "net_socket.h"
#include "port_scanner.h"

namespace {

std::vector<DashboardRow> CollectDashboardRows(uint16_t self_port) {
    const std::vector<PortEntry> ports = GetListeningPorts();
    std::vector<std::future<HttpProbeResult>> futures;
    futures.reserve(ports.size());

    for (const auto& port_entry : ports) {
        if (port_entry.port == self_port) {
            std::promise<HttpProbeResult> promise;
            promise.set_value(HttpProbeResult{});
            futures.push_back(promise.get_future());
        } else {
            const uint16_t port = port_entry.port;
            futures.push_back(std::async(std::launch::async, [port]() {
                return ProbeHttpPort(port);
            }));
        }
    }

    std::vector<DashboardRow> rows;
    rows.reserve(ports.size());
    for (size_t i = 0; i < ports.size(); ++i) {
        rows.push_back(DashboardRow{ ports[i], futures[i].get() });
    }
    return rows;
}

} // namespace

int main() {
    SocketRuntime runtime;

    uint16_t listen_port = 80;
    socket_t server_fd = CreateListener("127.0.0.1", listen_port);
    if (server_fd == kInvalidSocket) {
        listen_port = 8080;
        server_fd = CreateListener("127.0.0.1", listen_port);
    }
    if (server_fd == kInvalidSocket) {
        return 1;
    }

    std::cout << "Port Dashboard listening on http://127.0.0.1:" << listen_port << "\n";

    while (true) {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);
        const socket_t client_fd = accept(server_fd, reinterpret_cast<sockaddr*>(&client_addr), &client_len);
        if (client_fd == kInvalidSocket) {
            continue;
        }

        char buffer[4096];
        const int received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (received > 0) {
            buffer[received] = '\0';
            std::string_view request(buffer, static_cast<size_t>(received));

            if (request.starts_with("GET /favicon.ico")) {
                constexpr std::string_view kNoContent =
                    "HTTP/1.1 204 No Content\r\n"
                    "Connection: close\r\n\r\n";
                send(client_fd, kNoContent.data(), static_cast<int>(kNoContent.size()), 0);
                CloseSocket(client_fd);
                continue;
            }
        }

        const std::vector<DashboardRow> rows = CollectDashboardRows(listen_port);
        const std::string response = RenderDashboardHttpResponse(rows, listen_port);

        send(client_fd, response.data(), static_cast<int>(response.size()), 0);
        CloseSocket(client_fd);
    }

    CloseSocket(server_fd);
    return 0;
}