#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Reciever.h"
#include "Protocol.h"
#include <WinSock2.h>
#include <thread>
#include <fstream>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

int fragmentLength;

struct header analyzeHeader(char* buffer) {

    struct header protocol;

    protocol = *(struct header*)buffer;

    fragmentLength = protocol.data_len;

    return protocol;
}

void recieve(SOCKET listenSocket, struct sockaddr_in host) {

    char buffer[512];
    int slen = sizeof(host);

    std::ofstream fileOut;    
    
    while (true) {

        ZeroMemory(&buffer, 512);

        if ((recvfrom(listenSocket, (char*)buffer, 512, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            std::cout << "Error : recv failed.. shutting down!";
            return;
        }
        else {

            struct header protocol = analyzeHeader(buffer);


            

            printf("Received packet from %s:%d\n", inet_ntoa(host.sin_addr), ntohs(host.sin_port));
            
            printf("Data: %s", buffer);

        }

    }
}


void Reciever::wakeUp()
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

        return;

    }

    else

    {

        printf("The dll supports the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion),

            HIBYTE(wsaData.wVersion));

        printf("The highest version this dll can support: %u.%u\n", LOBYTE(wsaData.wHighVersion),

            HIBYTE(wsaData.wHighVersion));



        // Setup Winsock communication code here



        // When your application is finished call WSACleanup

        

        return;

    }
}

void Reciever::run() {

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    unsigned char buffer[512];
    int slen, recieved_len;

    struct sockaddr_in host;
    slen = sizeof(host);
    listenSocket = socket(socketi.sin_family, SOCK_DGRAM, IPPROTO_UDP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "ERROR : Bad socket init." << std::endl;
    }

    int iResult = bind(listenSocket, (struct sockaddr*)&socketi, sizeof(socketi));
    if (iResult == SOCKET_ERROR) {
        std::cout << "ERROR : bind error !!" << std::endl;
    }



    std::cout << "Server : Waiting for transfer..." << std::endl;
    while (true) {

        ZeroMemory(&buffer, sizeof(512));

        if ((recvfrom(listenSocket, (char*)buffer, 512, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            std::cout << "Error : recv failed";
            return;
        }
        else {

            printf("Received packet from %s:%d\n", inet_ntoa(host.sin_addr), ntohs(host.sin_port));
            printf("Data: %s\n", buffer);
        }




    }


    if (listenSocket == INVALID_SOCKET) {
        printf("Error : at socket(): %ld\n", WSAGetLastError());

        WSACleanup();
        return;
    }



}