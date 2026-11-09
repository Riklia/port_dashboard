#pragma once

#include <string>
#include <vector>

#include "port_scanner.h"
#include "http_probe.h"

struct DashboardRow {
    PortEntry port_info;
    HttpProbeResult http_info;
};

// Returns a complete HTTP/1.1 200 OK response containing the HTML dashboard
std::string RenderDashboardHttpResponse(const std::vector<DashboardRow>& rows, uint16_t self_port);
