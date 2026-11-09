cmake_minimum_required (VERSION 3.8)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

project ("port_dashboard")

set(SOURCES
    "src/main.cpp"
    "src/net_socket.h"
    "src/net_socket.cpp"
    "src/port_scanner.h"
    "src/http_probe.h"
    "src/http_probe.cpp"
    "src/dashboard_render.h"
    "src/dashboard_render.cpp"
    "src/dashboard.html"
)

if (WIN32)
    list(APPEND SOURCES "src/port_scanner_win.cpp")
elseif (UNIX AND NOT APPLE)
    list(APPEND SOURCES "src/port_scanner_linux.cpp")
endif()

add_executable(port_dashboard ${SOURCES})

configure_file("src/dashboard.html" "${CMAKE_CURRENT_BINARY_DIR}/dashboard.html" COPYONLY)
target_compile_definitions(port_dashboard PRIVATE DASHBOARD_TEMPLATE_FALLBACK="${CMAKE_CURRENT_SOURCE_DIR}/src/dashboard.html")

install(TARGETS port_dashboard RUNTIME DESTINATION bin)
install(FILES "src/dashboard.html" DESTINATION bin)

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET port_dashboard PROPERTY CXX_STANDARD 20)
endif()

# Link Windows socket and IP Helper libraries when building on Windows
if (WIN32)
  target_link_libraries(port_dashboard PRIVATE ws2_32 iphlpapi)
endif()