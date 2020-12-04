#pragma once
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <string>
#include <fstream>
#include <mutex>
#include <fstream>

#include "include/checksum.h"

#define MAX_SEND 100
#define ACK 10
#define KA 16
#define WINDOW 120

#define CLIENT_INIT(address, port ) \
  Sender( address, port)


class Sender
{


	public:
		Sender(unsigned long WSAAPI addr, unsigned short _port) {
			ip = addr;
			port = _port;
			alive = true;
			
			fragment = 1000;
			hostsockaddr.sin_family = AF_INET; 
			hostsockaddr.sin_port = htons(_port);
			hostsockaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

		}

		struct addrinfo* result = NULL,
						* ptr = NULL,
						hints;

		struct sockaddr_in hostsockaddr;

		int port;
		unsigned short fragment;
		static constexpr int maxFragment = 1000;
		unsigned long ip;
		static int sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket, int type);
		static int sendFile(std::string fileName, int fragmentLen, sockaddr_in hostsockaddr, SOCKET connectionSocket);
		static int connect(std::string fileName, int fragmentLength, sockaddr_in host, SOCKET socket);
		void wakeUp();
		void run();
		void cleanup();
		bool alive;
		WSADATA wsaData;
};

