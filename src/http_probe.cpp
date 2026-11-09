#include "http_probe.h"
#include "net_socket.h" // for socket_t, kInvalidSocket, CloseSocket

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstring>
#include <string_view>

namespace {

void SetSocketTimeout(socket_t s, int timeout_ms) {
#ifdef _WIN32
    DWORD tv = static_cast<DWORD>(timeout_ms);
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&tv), sizeof(tv));
#else
    timeval tv {
        .tv_sec = timeout_ms / 1000, 
        .tv_usec = (timeout_ms % 1000) * 1000 
    };
    setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    setsockopt(s, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
#endif
}

size_t FindCaseInsensitive(std::string_view haystack, std::string_view needle) {
    auto it = std::search(
        haystack.begin(), haystack.end(),
        needle.begin(), needle.end(),
        [](unsigned char a, unsigned char b) { return std::tolower(a) == std::tolower(b); }
    );
    return (it == haystack.end()) ? std::string_view::npos : static_cast<size_t>(it - haystack.begin());
}

std::string Trim(std::string_view string) {
    while (!string.empty() && std::isspace(static_cast<unsigned char>(string.front()))) {
        string.remove_prefix(1);
    }
    while (!string.empty() && std::isspace(static_cast<unsigned char>(string.back()))) {
        string.remove_suffix(1);
    }
    return std::string(string);
}

std::string FetchHttpResponse(uint16_t port, int timeout_ms) {
    const socket_t sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == kInvalidSocket) {
        return {};
    }

    SetSocketTimeout(sock, timeout_ms);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        CloseSocket(sock);
        return {};
    }

    const std::string request =
        "GET / HTTP/1.0\r\n"
        "Host: 127.0.0.1:" + std::to_string(port) + "\r\n"
        "User-Agent: PortDashboard/1.0\r\n"
        "Connection: close\r\n\r\n";

    if (send(sock, request.data(), static_cast<int>(request.size()), 0) < 0) {
        CloseSocket(sock);
        return {};
    }

    std::string response;
    char buf[4096];
    while (response.size() < 8192) {
        int bytes = recv(sock, buf, sizeof(buf), 0);
        if (bytes <= 0) {
            break;
        }
        response.append(buf, static_cast<size_t>(bytes));
    }
    CloseSocket(sock);
    return response;
}

int ExtractStatusCode(std::string_view response) {
    auto space = response.find(' ');
    if (space == std::string_view::npos) {
        return 0;
    }
    std::string_view code_view = response.substr(space + 1);
    int status_code = 0;
    std::from_chars(code_view.data(), code_view.data() + code_view.size(), status_code);
    return status_code;
}

std::string ExtractHeader(std::string_view headers, std::string_view header_prefix) {
    auto pos = FindCaseInsensitive(headers, header_prefix);
    if (pos == std::string_view::npos) {
        return {};
    }
    std::size_t val_start = pos + header_prefix.size();
    std::size_t line_end = headers.find("\r\n", val_start);
    return Trim(headers.substr(val_start, line_end - val_start));
}

std::string ExtractHtmlTitle(std::string_view html) {
    std::size_t title_open = FindCaseInsensitive(html, "<title");
    if (title_open == std::string_view::npos) {
        return {};
    }
    std::size_t tag_close = html.find('>', title_open);
    std::size_t title_end = FindCaseInsensitive(html, "</title>");
    if (tag_close == std::string_view::npos || title_end == std::string_view::npos || title_end <= tag_close) {
        return {};
    }
    return Trim(html.substr(tag_close + 1, title_end - (tag_close + 1)));
}

HttpProbeResult ParseHttpResponse(std::string_view response) {
    HttpProbeResult result;
    if (!response.starts_with("HTTP/")) {
        return result;
    }
    result.is_http = true;
    result.status_code = ExtractStatusCode(response);

    const std::size_t header_end = response.find("\r\n\r\n");
    std::string_view headers = (header_end != std::string_view::npos) ? response.substr(0, header_end) : response;

    result.server_header = ExtractHeader(headers, "\r\nServer:");
    result.page_title = ExtractHtmlTitle(response);
    return result;
}

} // namespace

HttpProbeResult ProbeHttpPort(uint16_t port, int timeout_ms) {
    const std::string response = FetchHttpResponse(port, timeout_ms);
    if (response.empty()) {
        return {};
    }
    return ParseHttpResponse(response);
}
