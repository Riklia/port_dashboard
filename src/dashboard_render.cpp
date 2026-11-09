#include "dashboard_render.h"

#include <fstream>
#include <sstream>
#include <string_view>
#include <unordered_set>

namespace {

constexpr std::string_view kTemplatePath = "dashboard.html";
constexpr std::string_view kRowsPlaceholder = "{{PORT_ROWS}}";

std::string EscapeHtml(std::string_view input) {
    std::string out;
    out.reserve(input.size());
    for (const char c : input) {
        switch (c) {
        case '&':  out += "&amp;"; break;
        case '<':  out += "&lt;"; break;
        case '>':  out += "&gt;"; break;
        case '"':  out += "&quot;"; break;
        case '\'': out += "&#39;"; break;
        default:   out += c; break;
        }
    }
    return out;
}

bool IsDefaultSystemProcess(const std::string& proc_name, uint16_t port) {
    static const std::unordered_set<std::string> kSystemProcs = {
        // Windows default noise
        "System", "Idle", "svchost.exe", "lsass.exe", "wininit.exe",
        "services.exe", "spoolsv.exe", "SearchIndexer.exe", "dashost.exe",
        // Linux / WSL default noise
        "sshd", "systemd", "systemd-resolve", "cupsd", "avahi-daemon", "init"
    };
    if (kSystemProcs.contains(proc_name)) return true;
    if (port >= 49152) {
        return true; // Windows ephemeral RPC range
    }
    return false;
}

std::string LoadHtmlTemplate() {
    std::ifstream file(kTemplatePath.data(), std::ios::binary);
#ifdef DASHBOARD_TEMPLATE_FALLBACK
    if (!file) {
        file.open(DASHBOARD_TEMPLATE_FALLBACK, std::ios::binary);
    }
#endif
    if (!file) {
        return R"(<html><body><h1>Error: Could not load dashboard.html</h1><table><tbody id="portTable">{{PORT_ROWS}}</tbody></table></body></html>)";
    }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string RenderPortCell(const DashboardRow& row, bool is_self) {
    const std::string port_str = std::to_string(row.port_info.port);
    std::string html = R"(<td><a class="port-link" href="http://127.0.0.1:)" + port_str +
                       R"(" target="_blank">:)" + port_str + "</a>";
    if (is_self) {
        html += R"(<span class="badge-self">THIS APP</span>)";
    } else if (row.http_info.is_http) {
        html += R"(<span class="badge-http">HTTP )" + std::to_string(row.http_info.status_code) + "</span>";
    }
    html += "</td>";
    return html;
}

std::string RenderMetaCell(const DashboardRow& row, bool is_self) {
    if (is_self) {
        return R"(<td class="meta">Port Dashboard</td>)";
    }
    if (!row.http_info.is_http) {
        return R"(<td class="meta"><span class="sub">-</span></td>)";
    }

    std::string inner;
    if (!row.http_info.page_title.empty()) {
        inner += "<strong>" + EscapeHtml(row.http_info.page_title) + "</strong> ";
    }
    if (!row.http_info.server_header.empty()) {
        inner += R"(<span class="sub">()" + EscapeHtml(row.http_info.server_header) + ")</span>";
    }
    if (inner.empty()) {
        inner = R"(<span class="sub">HTTP Server (no title)</span>)";
    }
    return R"(<td class="meta">)" + inner + "</td>";
}

std::string RenderRow(const DashboardRow& r, uint16_t self_port) {
    const uint16_t port = r.port_info.port;
    const bool is_self = (port == self_port);
    const bool is_sys = !is_self && IsDefaultSystemProcess(r.port_info.process_name, port);

    const std::string escaped_proc = EscapeHtml(r.port_info.process_name);
    const std::string escaped_cmd = EscapeHtml(r.port_info.command_line);

    std::ostringstream row;
    row << R"(<tr data-port=")" << port
        << R"(" data-proc=")" << escaped_proc
        << R"(" data-sys=")" << (is_sys ? "1" : "0") << R"(">)"
        << RenderPortCell(r, is_self)
        << RenderMetaCell(r, is_self)
        << "<td>" << escaped_proc << "</td>"
        << R"(<td class="sub">)" << r.port_info.pid << "</td>"
        << R"(<td class="cmd" title=")" << escaped_cmd << R"(">)" << escaped_cmd << "</td>"
        << R"(<td><button onclick="hidePort()" << port << R"html()" title="Hide this port">Hide</button></td>)html"
        << "</tr>\n";
    return row.str();
}

} // namespace

std::string RenderDashboardHttpResponse(const std::vector<DashboardRow>& rows, uint16_t self_port) {
    std::string rows_html;
    for (const DashboardRow& row : rows) {
        rows_html += RenderRow(row, self_port);
    }

    std::string body = LoadHtmlTemplate();
    if (const std::size_t pos = body.find(kRowsPlaceholder); pos != std::string::npos) {
        body.replace(pos, kRowsPlaceholder.size(), rows_html);
    }

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/html; charset=utf-8\r\n"
           "Content-Length: " + std::to_string(body.size()) + "\r\n"
           "Connection: close\r\n\r\n" +
           body;
}