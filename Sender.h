#pragma once
#include<WinSock2.h>
#define CLIENT_INIT(address, port ) \
  Sender( address, port)


class Sender
{
	public:
		Sender(unsigned long addr, int _port) {
			ip = addr;
			port = _port;
			alive = true;
		}

		struct addrinfo* result = NULL,
						* ptr = NULL,
						hints;


		int port;
		unsigned long ip;
		void wakeUp();
		void run();
		void cleanup();
		bool alive;
		WSADATA wsaData;
};

