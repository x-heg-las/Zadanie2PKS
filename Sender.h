#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <string>

#define CLIENT_INIT(address, port ) \
  Sender( address, port)


class Sender
{
	public:
		Sender(unsigned long WSAAPI addr, unsigned short _port) {
			ip = addr;
			port = _port;
			alive = true;
			

			hostsockaddr.sin_family = AF_INET; // mozno len ipv4
			hostsockaddr.sin_port = htons(_port);
			hostsockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		}

		struct addrinfo* result = NULL,
						* ptr = NULL,
						hints;

		struct sockaddr_in hostsockaddr;

		int port;
		static constexpr int maxFragment = 512;
		unsigned long ip;
		static int sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket);
		void wakeUp();
		void run();
		void cleanup();
		bool alive;
		WSADATA wsaData;
};

