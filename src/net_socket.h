#pragma once

#include <cstdint>
#include <iostream>
#include <vector>

#ifdef _WIN32
	#ifndef WIN32_LEAN_AND_MEAN
		#define WIN32_LEAN_AND_MEAN
	#endif
	#include <winsock2.h>
	#include <ws2tcpip.h>
#else
	#include <cerrno>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

#ifdef _WIN32
	using socket_t = SOCKET;
	constexpr socket_t kInvalidSocket = INVALID_SOCKET;
	inline void CloseSocket(socket_t s) { closesocket(s); }
	inline int GetLastSocketError() { return WSAGetLastError(); }
#else
	using socket_t = int;
	constexpr socket_t kInvalidSocket = -1;
	inline void CloseSocket(socket_t s) { close(s); }
	inline int GetLastSocketError() { return errno; }
#endif

struct SocketRuntime {
    SocketRuntime() {
#ifdef _WIN32
    WSADATA wsa_data{};
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed\n";
    }
#endif
    }
	SocketRuntime(SocketRuntime&) = delete;
	SocketRuntime& operator=(SocketRuntime&) = delete;
    SocketRuntime(SocketRuntime&&) = delete;
    SocketRuntime&& operator=(SocketRuntime&&) = delete;
    ~SocketRuntime() {
#ifdef _WIN32
        WSACleanup();
#endif
    }
};

socket_t CreateListener(const char* ip, uint16_t port);
