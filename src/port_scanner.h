#pragma once

#include <cstdint>
#include <string>
#include <vector>

struct PortEntry {
    uint16_t port = 0;
    uint32_t pid = 0;
    std::string process_name;
    std::string command_line;
};

std::vector<PortEntry> GetListeningPorts();