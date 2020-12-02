#include "Sender.h"
#include <WinSock2.h>
#include <iostream>
#include <thread>
#include "Protocol.h"
#include <windows.h>
#include <string>
#include "InitControll.h"
#include <fstream>
#include <string.h>
#include <mutex>



void recieveMessage(){
    
}

int Sender::connect(std::string filePath, int fragmentLength, sockaddr_in host, SOCKET socket) {

    struct fragment stream;
   

    ZeroMemory(&stream, sizeof(stream));
    char* fileBuffer = new char[ MAX_PATH];
    ZeroMemory(fileBuffer,  MAX_PATH);
    int result;

    std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);

    memcpy(fileBuffer, fileName.c_str(), fileName.length() + 1);

    if (fileName.length() + 1 > fragmentLength) {
        stream.header.type.len = 3;
    }
    else
        stream.header.type.len = 2;

    fragmentMessage(stream, fileName.length(), fileBuffer, fragmentLength, NAME);

   

    while (stream.next) {

        if (result = sendto(socket, stream.data, (stream.header.dataLength + stream.header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

            std::cout << "Client : Failed to send data" << std::endl;
        }

        stream = *stream.next;
    }

    



    return 0;
}

int Sender::sendFile(std::string filePath, int fragmentLen, sockaddr_in hostsockaddr, SOCKET connectionSocket)
{

    std::ifstream fileInput(filePath, std::ifstream::in | std::ifstream::binary);
    
    struct fragment stream;
    fileInput.seekg(0, fileInput.end);
    int fileSize = fileInput.tellg() , result = SOCKET_ERROR;
    fileInput.seekg(0, fileInput.beg);

    ZeroMemory(&stream, sizeof(stream));
    char* fileBuffer = new char[fileSize + MAX_PATH];
    ZeroMemory(fileBuffer, fileSize + MAX_PATH);
   
    if (fileSize + HEADER_8 > fragmentLen) {
        
        stream.header.flags.fragmented = 1;
        stream.header.type.len = 3;
        stream.header.type.binary = 1;
    }
    else {
        stream.header.type.len = 2;
        stream.header.type.binary = 1;
    }

    
    fileInput.read(fileBuffer , static_cast<std::streamsize>(fileSize));

    fragmentMessage(stream, fileSize, fileBuffer, fragmentLen, FILE);

    int cnt = 0;
    while (stream.next) {

        if (result = sendto(connectionSocket, stream.data, (stream.header.dataLength + stream.header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {
        
            std::cout << "Client : Failed to send data" << std::endl;
        }
        Sleep(1);
        cnt++;
        stream = *stream.next;
    }

    std::cout << "POSLANYCH : " + cnt;
    // este neak uvolnovanie dat zabezpeciit 
    return 1;
}

int Sender::sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket, int type) // type nemusi byt
{
    int len = message.length();
    int result;
    struct fragment stream;
    ZeroMemory(&stream, sizeof(stream));

    if ((len+1) + HEADER_8 > fragmentLen) {

        stream.header.flags.fragmented = 1;
        stream.header.type.len = 3;
        stream.header.type.text = 1;
       
        fragmentMessage(stream, len+1, (char*)message.c_str(), fragmentLen, TEXTM);
        int cnt = 0;
        while (stream.next) {
            
            if (result = sendto(connectionSocket, stream.data, (stream.header.dataLength + stream.header.type.len*4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {
                
                std::cout << "Client :fragment send error" << std::endl;

            }
            cnt++;
            stream = *stream.next;
           
        }
        std::cout << "POSLANYCH : " + cnt;
        // spoj do jedneho prikzu
    }
    else {
        stream.header.type.len = 2;
        stream.header.type.text = 1;
        fragmentMessage(stream, len + 1, (char*)message.c_str(), fragmentLen, type);
        
        if (result = sendto(connectionSocket, stream.data, (stream.header.dataLength + stream.header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

            struct fragment messageHeader; 

            std::cout << "Client :fragment send error" << std::endl;

        }

      
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
    char recieveBuffer[1000];

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
    
     while (true) {
        char choice;

        std::cout << "help : [t] (text message) [f] (file) [e] (shutdown) [l] (nastav velkost fragmentu)" << std::endl;
        std::cin >> choice;
        std::string msg, filename;

        switch (choice) {
            case 't':
                std::cout << "Message : ";
                std::cin >> msg;
                _resullt = sendMessage(msg, fragment, hostsockaddr, connectionSocket, TEXTM);
                break;
            case 'f' :
                filename = getFilename();
                
                _resullt = connect(filename, fragment, hostsockaddr, connectionSocket);

                _resullt = sendFile(filename, fragment, hostsockaddr, connectionSocket);
               

                break;
            case 'l':
                std::cin >> fragment;
                break;
            case 'e':
                closesocket(connectionSocket);
                std::cout << "Client : client is shutting downn" << std::endl;
                return;

        }

        if (_resullt == SOCKET_ERROR) {
            std::cout << "Error : client is shutting down" << std::endl;
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
