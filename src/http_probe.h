#pragma once

#include <cstdint>
#include <string>

struct HttpProbeResult {
    bool is_http = false;
    int status_code = 0;
    std::string server_header;
    std::string page_title;
};

// Probes 127.0.0.1:port with a short timeout (default 150ms).
HttpProbeResult ProbeHttpPort(uint16_t port, int timeout_ms = 150);
