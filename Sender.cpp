#include "Sender.h"
#include <WinSock2.h>
#include <iostream>
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



        // When your application is finished call WSACleanup

        if (WSACleanup() == SOCKET_ERROR)

            printf("WSACleanup failed with error %d\n", WSAGetLastError());

        // and exit

        return ;

    }
}

void Sender::run()
{

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;
}

void Sender::cleanup()
{

	if (WSACleanup() == SOCKET_ERROR)
	{
		std::cout << "Error : WSACleanup failed " + WSAGetLastError();
	}
}
