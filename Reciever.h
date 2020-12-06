#pragma once
#include<WinSock2.h>
#include<iostream>

class Reciever
{
public:
	Reciever(unsigned short port) {

		alive = true;
		socketi.sin_family = AF_INET;
		socketi.sin_port = htons(port);
		socketi.sin_addr.s_addr = inet_addr("127.0.0.1");
	}

	struct sockaddr_in socketi;
	void wakeUp();
	void run();
	void cleanup();
	bool alive;
	WSADATA wsaData;

};

