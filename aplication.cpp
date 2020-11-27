#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<iostream>
#include<WinSock2.h>
#include"InitControll.h"
#include"Sender.h"
#include "Reciever.h"

#pragma comment(lib, "Ws2_32.lib") // alebo zmenit v konfiguraciach

int main(int argc, char **argv) {

	int port;
	char val;
	SOCKET socket;
	SOCKADDR_IN recieverAddress;

	std::cout << "Spustit ako : [sender(s) / reciever(r)]" << std::endl;
	
	for (val = BAD_INPUT; val == BAD_INPUT; val = chooseService());


	if (val == RECIEVER) 
	{
	
		Reciever server = Reciever(48514);

		server.wakeUp();
		server.run();


	}
	else if (val == SENDER) 
	{

		//getFilename();
		
		unsigned long WSAAPI IP_addr;

		for(IP_addr = INADDR_NONE; IP_addr == INADDR_NONE; IP_addr = inet_addr("127.0.0.1"));  //TODO : skontroluj riadne to nacitavanie

		Sender client = CLIENT_INIT(IP_addr, 48514);
		
		client.wakeUp();
		client.run();
		
	}
}