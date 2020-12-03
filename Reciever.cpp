#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Reciever.h"
#include "InitControll.h"
#include "Protocol.h"
#include <WinSock2.h>
#include <Windows.h>
#include <thread>
#include <fstream>
#include <vector>
#include <iostream>

#define MAX_FILE 2100000
#pragma comment(lib,"ws2_32.lib")

bool reply(sockaddr_in host, sockaddr_in server, char* data) {

    header protocol;
    analyzeHeader(protocol, data);

    unsigned short sum = crc(data);
    SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

    int seq;
    char size = protocol.type.len;;

    if (protocol.type.len == 3)
        seq = protocol.seq;
    else
        seq = -1;

    ZeroMemory(&protocol, sizeof(protocol));
    protocol.type.len = size;

    if (!sum) {
        protocol.flags.ack = 1;
        sendto(client, arq(protocol, seq), 12, 0, (sockaddr*)&host, sizeof(host));
        return true;
    }
    else {
       
        protocol.flags.retry = 1;
        sendto(client, arq(protocol, seq), 12, 0, (sockaddr*)&host, sizeof(host));
        return false;
    }
}

void recieve(SOCKET listenSocket, struct sockaddr_in socketi) {

    char* buffer = new char [MAX_FILE];
    int slen ;
    char* filebuffer = new char[MAX_FILE];
     
    int c = 0;

    sockaddr_in host;
    slen = sizeof(host);
    header protocol, reference;
    message transfered;
    stream* dataFlow;
       
    char* fileName = nullptr;
    short filenameSize = 0;
    message* ptr = &transfered;
    std::vector<Stream> namevec;
    std::vector<Stream> streams; 

    ZeroMemory(&dataFlow, sizeof(dataFlow));
    while (true) {
        
        ZeroMemory(buffer, 1000);

        if ((recvfrom(listenSocket, (char*)buffer, 1000, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            std::cout << "Server : connection lost";
            return;
        }
        else {
            
           
            printf("Received packet from %s:%d\n", inet_ntoa(host.sin_addr), ntohs(host.sin_port));

            analyzeHeader(protocol, buffer);
    
            if (protocol.flags.init) {
                ptr = &transfered;
                reference = protocol; 
                Stream incoming = Stream(reference.stream, 0);
                
                if(protocol.flags.name)
                    namevec.push_back(incoming);
                else
                    streams.push_back(incoming);

            }

            if (protocol.type.control) {

                if (reply(host, host, buffer)) {

                   std::cout << "Server : packet: " << ntohs(host.sin_port) << ":seq:" << protocol.seq << " ->OK" << std::endl;
                    if (protocol.type.binary && protocol.flags.name) {
                        Stream *str; 

                        str = findStream(namevec, protocol.stream);
                         
                        str->addFragment(Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq));
                        
                        //neviem ci to je potrebne 
                        filenameSize += protocol.data_len;

                        if (protocol.flags.quit ) {
                            str->initializeMissing(protocol.seq);
                            for (int i = 0; i < 2; i++) {
                                if (!checkCompletition(namevec, protocol.stream)) {

                                    std::vector<char> missing = requestPackets(*str);
                                    header reference;
                                    ZeroMemory(&reference, sizeof(reference));

                                    reference.flags.retry = reference.type.control = reference.type.binary = 1;
                                    for (auto member : missing)
                                    {
                                        reply(host, host, arq(reference, reference.seq));
                                    }

                                    std::this_thread::sleep_for(std::chrono::microseconds(50));
                                }
                                else
                                    break;
                            }
                        }
                    }
                }
                else {
                    std::cout << "Server : packet: " << ntohs(host.sin_port) << ":seq:" << protocol.seq << " ->BAD" << std::endl;
                    continue;
                }
                if(protocol.flags.name || !(protocol.type.text || protocol.type.binary))
                    continue;
            }
            
            if (!protocol.type.control) {
                reply(host, host, buffer);
                std::cout << "Server : packet: " << ntohs(host.sin_port) << ":seq:" << protocol.seq << " ->OK" << std::endl;
            }
      
            if (!protocol.flags.quit) {

                    Stream *str; 
                    str = findStream(streams, protocol.stream);
                    str->addFragment(Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq));                
            }
            else {                 
                //** prijmeme posledny paket komunikacie **//

                   
                Stream *str = findStream(streams, protocol.stream);
                str->addFragment(Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq));
                    

                //////// toto oprav na zrozumitelnejsie ////////
                str->initializeMissing(protocol.seq);
                for(int i = 0; i < 2; i++){
                    if (checkCompletition(streams, protocol.stream)) {

                                                                        std::cout << "saving" << std::endl;

                        if (protocol.type.binary) {
                            fileName = new char[filenameSize + 1];
                               
                            concat(namevec, protocol.stream, fileName);
                            fileName[filenameSize] = '\0';
                               
                            int size = concat(streams, protocol.stream, filebuffer);
                                
                                
                            //vytvorenie suboru
                            std::ofstream file(fileName, std::ios::out | std::ios::binary | std::ios::app);
                            file.write(filebuffer, size);
                            file.close();


                            delete[] fileName;
                                
                        }

                        if (protocol.type.text) {
                            ZeroMemory(buffer, 512);
                            concat(streams, protocol.stream, buffer);
                            std::cout << buffer << std::endl;
                        }

                        break;
                    }
                    else {      // znovuvyziadanie spravy


                        std::vector<char> missing = requestPackets(*str);
                        header reference;
                        ZeroMemory(&reference, sizeof(reference));

                        reference.flags.retry = reference.type.control = reference.type.binary = 1;

                        for (auto member : missing)

                        {
                            reply(host, host, arq(reference, reference.seq));
                        }

                        Sleep(150); /// what ? :D
                    }
                }                  
            }
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
    std::thread recieving(&recieve, listenSocket, host);
    
    
    while (alive) {
        int input;
        std::cin >> input;

        if (input == 0)
            alive = false;
    }
    
    if (recieving.joinable())
        closesocket(listenSocket);
        recieving.join();

    if (listenSocket == INVALID_SOCKET) {
        printf("Error : at socket(): %ld\n", WSAGetLastError());

        WSACleanup();
        return;
    }



}