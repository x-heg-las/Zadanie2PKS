#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <string>
#include <fstream>

#define FILE 1
#define TEXT 2

#define CLIENT_INIT(address, port ) \
  Sender( address, port)


class Sender
{
	public:
		Sender(unsigned long WSAAPI addr, unsigned short _port) {
			ip = addr;
			port = _port;
			alive = true;
			
			fragment = 512;
			hostsockaddr.sin_family = AF_INET; // mozno len ipv4
			hostsockaddr.sin_port = htons(_port);
			hostsockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		}

		struct addrinfo* result = NULL,
						* ptr = NULL,
						hints;

		struct sockaddr_in hostsockaddr;

		int port;
		unsigned short fragment;
		static constexpr int maxFragment = 512;
		unsigned long ip;
		static int sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket, int type);
		static int sendFile(std::string fileName, int fragmentLen, sockaddr_in hostsockaddr, SOCKET connectionSocket);
		void wakeUp();
		void run();
		void cleanup();
		bool alive;
		WSADATA wsaData;
};

