#ifndef _WIN32

#include "port_scanner.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <set>
#include <algorithm>

namespace fs = std::filesystem;

namespace {

struct ProcInfo {
    uint32_t pid = 0;
    std::string name;
    std::string cmdline;
};

ProcInfo ParseProcInfoFromPath(const fs::path path, uint32_t pid) {
    // Read process name (/proc/<pid>/comm)
    std::string comm;
    std::ifstream comm_file(path / "comm");
    std::getline(comm_file, comm);

    // Read command line (/proc/<pid>/cmdline, null-separated)
    std::string cmdline;
    std::ifstream cmd_file(path / "cmdline", std::ios::binary);
    std::getline(cmd_file, cmdline, '\0');

    return { pid, comm, cmdline };
}

std::unordered_map<uint64_t, ProcInfo> MapInodesToProcesses() {
    std::unordered_map<uint64_t, ProcInfo> inode_map;

    for (const auto& proc_entry : fs::directory_iterator("/proc", fs::directory_options::skip_permission_denied)) {
        if (!proc_entry.is_directory()) {
            continue;
        }

        const std::string pid_str = proc_entry.path().filename().string();
        if (!std::all_of(pid_str.begin(), pid_str.end(), ::isdigit)) {
            continue;
        }

        const uint32_t pid = static_cast<uint32_t>(std::stoul(pid_str));
        const fs::path fd_dir = proc_entry.path() / "fd";

        std::error_code error_code;
        for (const auto& fd_entry : fs::directory_iterator(fd_dir, fs::directory_options::skip_permission_denied, error_code)) {
            if (!fd_entry.is_symlink(error_code)) {
                continue;
            }
            std::string target = fs::read_symlink(fd_entry.path(), error_code).string();
            // Target looks like: "socket:[12345]"
            if (target.starts_with("socket:[") && target.ends_with("]")) {
                const uint64_t inode = std::stoull(target.substr(8, target.size() - 9));
                inode_map[inode] = ParseProcInfoFromPath(proc_entry.path(), pid);
            }
        }
    }
    return inode_map;
}

void ParseProcNetTcp(const char* path,
    const std::unordered_map<uint64_t, ProcInfo>& inode_map,
    std::vector<PortEntry>& out,
    std::set<uint16_t>& seen_ports) {
    std::ifstream file(path);
    std::string line;
    std::getline(file, line); // Skip header line

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string sl, local_addr, rem_addr, state;
        iss >> sl >> local_addr >> rem_addr >> state;

        // 0A = TCP_LISTEN
        if (state != "0A") {
            continue; 
        }

        auto colon = local_addr.find(':');
        if (colon == std::string::npos) {
            continue;
        }
        const uint16_t port = static_cast<uint16_t>(std::stoul(local_addr.substr(colon + 1), nullptr, 16));

        // Avoid duplicates (e.g. IPv4 + IPv6)
        if (!seen_ports.insert(port).second) {
            continue;
        }

        std::string dummy;
        uint64_t inode = 0;
        iss >> dummy >> dummy >> dummy >> dummy >> dummy >> inode;

        PortEntry entry{ port, 0, "unknown (other user/root)", "" };
        if (auto it = inode_map.find(inode); it != inode_map.end()) {
            entry.pid = it->second.pid;
            entry.process_name = it->second.name;
            entry.command_line = it->second.cmdline;
        }
        out.push_back(std::move(entry));
    }
}
}  // namespace

std::vector<PortEntry> GetListeningPorts() {
    std::vector<PortEntry> results;
    std::set<uint16_t> seen_ports;
    auto inode_map = MapInodesToProcesses();
    ParseProcNetTcp("/proc/net/tcp", inode_map, results, seen_ports);
    ParseProcNetTcp("/proc/net/tcp6", inode_map, results, seen_ports);
    std::sort(results.begin(), results.end(),
        [](const PortEntry& a, const PortEntry& b) { return a.port < b.port; });
    return results;
}

#endif
