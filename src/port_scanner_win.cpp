#ifdef _WIN32
#include "port_scanner.h"

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iphlpapi.h>
#include <algorithm>
#include <set>
#include <filesystem>

namespace {

std::string GetProcessNameByPid(DWORD pid, std::string& out_path) {
    if (pid == 0) {
        return "Idle";
    }
    if (pid == 4) {
        return "System";
    }

    HANDLE process_handle = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process_handle) {
        return "Access Denied / System";
    }

    char buf[MAX_PATH];
    DWORD size = MAX_PATH;
    std::string name = "unknown";
    if (QueryFullProcessImageNameA(process_handle, 0, buf, &size)) {
        out_path = buf;
        name = std::filesystem::path(buf).filename().string();
    }
    CloseHandle(process_handle);
    return name;
}

void QueryWindowsTcpTable(ULONG family, std::vector<PortEntry>& out, std::set<uint16_t>& seen_ports) {
    ULONG buf_size = 0;
    // First call gets required buffer size
    GetExtendedTcpTable(nullptr, &buf_size, FALSE, family, TCP_TABLE_OWNER_PID_LISTENER, 0);
    if (buf_size == 0) return;

    std::vector<uint8_t> buffer(buf_size);
    if (GetExtendedTcpTable(buffer.data(), &buf_size, FALSE, family, TCP_TABLE_OWNER_PID_LISTENER, 0) != NO_ERROR) {
        return;
    }

    if (family == AF_INET) {
        auto* table = reinterpret_cast<MIB_TCPTABLE_OWNER_PID*>(buffer.data());
        for (DWORD i = 0; i < table->dwNumEntries; ++i) {
            const auto& row = table->table[i];
            uint16_t port = ntohs(static_cast<u_short>(row.dwLocalPort));
            if (!seen_ports.insert(port).second) continue;

            std::string full_path;
            std::string name = GetProcessNameByPid(row.dwOwningPid, full_path);
            out.push_back(PortEntry{ port, row.dwOwningPid, name, full_path });
        }
    }
    else if (family == AF_INET6) {
        auto* table = reinterpret_cast<MIB_TCP6TABLE_OWNER_PID*>(buffer.data());
        for (DWORD i = 0; i < table->dwNumEntries; ++i) {
            const auto& row = table->table[i];
            uint16_t port = ntohs(static_cast<u_short>(row.dwLocalPort));
            if (!seen_ports.insert(port).second) {
                continue;
            }

            std::string full_path;
            std::string name = GetProcessNameByPid(row.dwOwningPid, full_path);
            out.push_back(PortEntry{ port, row.dwOwningPid, name, full_path });
        }
    }
}
}  // namespace

std::vector<PortEntry> GetListeningPorts() {
    std::vector<PortEntry> results;
    std::set<uint16_t> seen_ports;
    QueryWindowsTcpTable(AF_INET, results, seen_ports);
    QueryWindowsTcpTable(AF_INET6, results, seen_ports);
    std::sort(results.begin(), results.end(),
        [](const PortEntry& a, const PortEntry& b) { return a.port < b.port; });
    return results;
}

#endif