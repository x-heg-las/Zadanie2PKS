#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include<iostream>
#include<WinSock2.h>
#include"InitControll.h"
#include <thread>
#include"Sender.h"
#include <atomic>
#include "Reciever.h"


#pragma comment(lib, "Ws2_32.lib") // alebo zmenit v konfiguraciach

int main(int argc, char **argv) {

	unsigned short port = 0;
	char val;
	bool running = true;

	while (running) {
		std::cout << "Spustit ako : [sender(s) / reciever(r) / ukoncit(u)]" << std::endl;

		for (val = BAD_INPUT; val == BAD_INPUT; val = chooseService());


		if (val == RECIEVER) {

			for (port = 0; port < 1024 || port > 65535; port = loadPort());

			Reciever server = Reciever(port);

			server.wakeUp();
			server.run();
			server.cleanup();

		}
		else if (val == SENDER) {

			unsigned long WSAAPI IP_addr;

			for (IP_addr = INADDR_NONE; IP_addr == INADDR_NONE; IP_addr = inet_addr(loadIP().c_str()));  //TODO : skontroluj riadne to nacitavanie
			for (port = 0; port < 1024 || port > 65535; port = loadPort());

			Sender client = Sender(IP_addr, port);

			client.wakeUp();
			client.run();
			client.cleanup();
		}
		else if (val == 'u')
			running = false;

	}
	
	return 0;
}