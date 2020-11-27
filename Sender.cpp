#include "Sender.h"
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include "Protocol.h"
#include <string>
#include "InitControll.h"



int Sender::sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket)
{
    int len = message.length();
    int result;
    struct fragment *stream = NULL;

    if (len > fragmentLen) {
        stream = fragmentMessage(stream, len, (char*)message.c_str(), fragmentLen);

        while (stream) {
            if (result = sendto(connectionSocket, stream->data, stream->header.dataLength, 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {


            }
            
             

            stream = stream->next;
        }

    }
    else {

    }
    return 0;
}

void Sender::wakeUp()
{


	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		std::cout << "Error : WSAStartup failed " + WSAGetLastError();
		alive = false;
		return;
	}

    else
    {

        printf("The Winsock dll found!\n");

        printf("The current status is: %s.\n", wsaData.szSystemStatus);

    }




    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
    {

        //Tell the user that we could not find a usable WinSock DLL

        printf("The dll do not support the Winsock version %u.%u!\n",

            LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));

        // When your application is finished call WSACleanup

        WSACleanup();

        // and exit

        return ;

    }

    else

    {

        printf("The dll supports the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion),

            HIBYTE(wsaData.wVersion));

        printf("The highest version this dll can support: %u.%u\n", LOBYTE(wsaData.wHighVersion),

            HIBYTE(wsaData.wHighVersion));

        // Setup Winsock communication code here

        return ;

    }
}

void Sender::run()
{
    int _resullt = 0;

    const char * message = "this is a mesage that i want to send bla bah blah baaa je tiooto skoro koniec uzz teraz fakt. tu :D";
    char recieveBuffer[512];

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    SOCKET connectionSocket = INVALID_SOCKET;
    
    connectionSocket = socket(hints.ai_family, hints.ai_socktype, 0);
    
    if (connectionSocket == INVALID_SOCKET) // AF_UNSPEC moze padat 
    {
        std::cout << "Error : at socket " + WSAGetLastError();
        WSACleanup();
        return;
    }
    
   // _resullt = connect(connectionSocket, (sockaddr *)&hostsockaddr.sin_addr, sizeof(hostsockaddr));

    while (true) {
        char choice;

        std::cout << "help : [t] (text message) [f] (file) [e] (shutdown)" << std::endl;
        std::cin >> choice;
        std::string msg;

        switch (choice) {
            case 't':
                std::cout << "Message : ";
                std::cin >> msg;
                _resullt = sendMessage(msg, 32, hostsockaddr, connectionSocket);
                break;
            case 'f' :

                break;

        }



    }

  //send the message
    if (sendto(connectionSocket, message, strlen(message), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR)
    {
        printf("sendto() failed with error code : %d", WSAGetLastError());
        exit(EXIT_FAILURE);
    }

    if (_resullt == SOCKET_ERROR)
    {
       _resullt =  WSAGetLastError();
        closesocket(connectionSocket);
        connectionSocket = INVALID_SOCKET;
    }
    else 
    {
        std::cout << "Client : connected to server" << std::endl;
    


    }

    if (connectionSocket == INVALID_SOCKET)
    {
        std::cout << "Unable to connect to reciever" << std::endl;
        WSACleanup();
        return;
    }

    
}

void Sender::cleanup()
{

	if (WSACleanup() == SOCKET_ERROR)
	{
		std::cout << "Error : WSACleanup failed " + WSAGetLastError();
	}
}
