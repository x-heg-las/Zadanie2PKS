#include<iostream>
#include<WinSock2.h>
#include"InitControll.h"


int main(int argc, char **argv) {

	int port;
	char val;
	SOCKET socket;
	SOCKADDR_IN recieverAddress;

	

	std::cout << "Spustit ako : [sender(s) reciever(r)]" << std::endl;
	
	for (val = BAD_INPUT; val == BAD_INPUT; val = chooseService());


	if (val == RECIEVER) 
	{

		


	}
	else if (val == SENDER) 
	{
		
		IPv4_ADDR  IP_addr;

		for(IP_addr = INADDR_NONE; IP_addr == INADDR_NONE; IP_addr = inet_addr(loadIP().c_str()));  //TODO : skontroluj riadne to nacitavanie


		
		

		
	}
}