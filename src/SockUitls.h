#pragma once

#include "glm.hpp"

#include <string>

#ifdef _WIN32 // windows specific socket include

#include <WinSock2.h>
#include <WS2tcpip.h>

#elif __linux__

#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

typedef in_addr IN_ADDR;
typedef in6_addr IN6_ADDR;

#endif

namespace sock {
	inline int closeSocket(int socket) {
#ifdef _WIN32
		return closesocket(socket);
#elif __linux__
		return close(socket);
#endif
	}

	inline int pollState(pollfd fds[], size_t nfds, int timeout) {
#ifdef _WIN32
		return WSAPoll(fds, nfds, timeout);
#elif __linux__
		return poll(fds, nfds, timeout);
#endif
	}

	// host network conversion
	inline void htonMat4(const glm::mat4& mat4, void* nData) {
		uint32_t* buf = reinterpret_cast<uint32_t*>(nData);
		for (size_t i = 0; i < 16; i += 1) {
			uint32_t nf = htonf(mat4[floor(i / 4)][i % 4]);
			buf[i] = nf;
		}
	}

	inline void ntohMat4(const void* nData, glm::mat4& mat4) {
		const uint32_t* buf = reinterpret_cast<const uint32_t*>(nData);
		for (size_t i = 0; i < 16; i += 1) {
			float hf = ntohf(buf[i]);
			mat4[floor(i / 4)][i % 4] = hf;
		}
	}

	inline void htonAddr(const sockaddr_storage& addr, void* nData) {
		uint16_t* shortData = reinterpret_cast<uint16_t*>(nData);
		shortData[0] = htons(addr.ss_family); shortData++;
	}

	inline void ntohAddr(const void* nData, sockaddr_storage& addr) {
		const uint16_t* shortData = reinterpret_cast<const uint16_t*>(nData);
		addr.ss_family = ntohs(shortData[0]); shortData++;
	}

	inline IN_ADDR presentationToAddrIPv4(std::string presentation) {
		IN_ADDR addr;
		if (inet_pton(AF_INET, presentation.c_str(), &addr) <= 0) {
			fprintf(stderr, "error while decoding ip address\n");
			exit(3);
		}
		return addr;
	}
	inline IN6_ADDR presentationToAddrIPv6(std::string presentation) {
		IN6_ADDR addr;
		if (inet_pton(AF_INET6, presentation.c_str(), &addr) <= 0) {
			fprintf(stderr, "error while decoding ip address\n");
			exit(3);
		}
		return addr;
	}

	inline std::string addrToPresentationIPv4(IN_ADDR addr) {
		char ip4[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addr, ip4, INET_ADDRSTRLEN);
		return std::string(ip4);
	}
	inline std::string addrToPresentationIPv6(IN6_ADDR addr) {
		char ip6[INET6_ADDRSTRLEN];
		inet_ntop(AF_INET6, &addr, ip6, INET6_ADDRSTRLEN);
		return ip6;
	}
	inline std::string addrToPresentation(sockaddr* sa) {
		if (sa->sa_family == AF_INET) {
			return addrToPresentationIPv4(reinterpret_cast<sockaddr_in*>(sa)->sin_addr);
		}
		return addrToPresentationIPv6(reinterpret_cast<sockaddr_in6*>(sa)->sin6_addr);
	}

	inline int cmpAddr(const sockaddr* a, const sockaddr* b) {
		if (a->sa_family != b->sa_family)
			return -1;

		if (a->sa_family == AF_INET) {
			sockaddr_in sa4a = *reinterpret_cast<const sockaddr_in*>(a);
			sockaddr_in sa4b = *reinterpret_cast<const sockaddr_in*>(b);
			return *(uint32_t*)(&sa4a.sin_addr) - *(uint32_t*)(&sa4b.sin_addr) +
				sa4a.sin_port - sa4b.sin_port;
		}
		else if (a->sa_family == AF_INET6) {
			sockaddr_in6 sa6a = *reinterpret_cast<const sockaddr_in6*>(a);
			sockaddr_in6 sa6b = *reinterpret_cast<const sockaddr_in6*>(b);
			return memcmp((char*)&sa6a, (char*)&sa6b, sizeof(sockaddr_in6));
		}
		else {
			printf("cmpAddr: unsupported address family %i\n", (int)a->sa_family);
			exit(0);
		}
	}

	inline int lastError() {
#ifdef _WIN32
		return WSAGetLastError();
#elif __linux__
		return errno;
#endif
	}

	inline void printLastError(const char* msg) {
#ifdef _WIN32
		char* s = NULL;
		FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, WSAGetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPSTR)&s, 0, NULL);
		fprintf(stderr, "%s: %s\n", msg, s);
#elif __linux__
		perror(msg);
#endif
	}
}
