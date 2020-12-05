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

#include "include/checksum.h"

#define MAX_FILE 2100000
#pragma comment(lib,"ws2_32.lib")

bool reply(sockaddr_in host, sockaddr_in server, char* data) {

    header protocol;
    analyzeHeader(protocol, data);

    unsigned short sum = crc(data, protocol.data_len + (protocol.type.len * 4));
    SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    unsigned short packetChecksum = protocol.chcecksum;
    int seq;
    char size = protocol.type.len;;

    if (protocol.type.len == 3)
        seq = protocol.seq;
    else
        seq = -1;


    if (protocol.type.keep_alive) {
        protocol.flags.ack = 1;
        sendto(client, arq(protocol, -1), HEADER_8, 0, (sockaddr*)&host, sizeof(host));
        return true;
    }

    ZeroMemory(&protocol, sizeof(protocol));
    protocol.type.len = size;



    if (sum == packetChecksum) {
        protocol.flags.ack = 1;
        sendto(client, arq(protocol, seq), HEADER_12, 0, (sockaddr*)&host, sizeof(host));
        return true;
    }
    else {
       
        protocol.flags.retry = 1;
        sendto(client, arq(protocol, seq), HEADER_12, 0, (sockaddr*)&host, sizeof(host));
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
    Stream* recent = nullptr;
       
    char* fileName = nullptr;
    short filenameSize = 0, lastStream = -1;

    message* ptr = &transfered;
    std::vector<Stream> namevec;
    std::vector<Stream> streams; 
 
    ZeroMemory(&dataFlow, sizeof(dataFlow));
    while (true) {
        
      //  ZeroMemory(buffer, 1000);

        if ((recvfrom(listenSocket, (char*)buffer, 1000, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            std::cout << "Server : connection lost";
            return;
        }
        else {

            printf("\nReceived packet from %s:%d", inet_ntoa(host.sin_addr), ntohs(host.sin_port));

            analyzeHeader(protocol, buffer);



            if (protocol.type.control) {

                if (protocol.flags.init) {
                    ptr = &transfered;
                    reference = protocol;
                    Stream incoming = Stream(reference.stream, 0);

                    if (protocol.flags.name && !protocol.flags.resesend)
                        namevec.push_back(incoming);
                    else if(!protocol.flags.resesend)
                        streams.push_back(incoming);

                }

                if (reply(host, host, buffer)) {

                    printf("\nServer : packet no. %d -> OK", protocol.seq);

                    if (protocol.type.binary && protocol.flags.name) {
                        Stream* str;

                        str = findStream(namevec, protocol.stream);
                        Message mesg = Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq);

                        str->addFragment(mesg);

                        //neviem ci to je potrebne 
                        filenameSize += protocol.data_len;

                        if (protocol.flags.quit) {
                            str->initializeMissing(protocol.seq);

                            if (!checkCompletition(namevec, protocol.stream)) {
                                /*
                                std::vector<char> missing = requestPackets(*str);
                                header reference;
                                ZeroMemory(&reference, sizeof(reference));

                                reference.flags.retry = reference.type.control = reference.type.binary = 1;
                                for (auto member : missing)
                                {
                                    reply(host, host, arq(reference, reference.seq));
                                }

                                std::this_thread::sleep_for(std::chrono::microseconds(50));
                                */
                            }


                        }
                    }
                    else if (protocol.type.keep_alive)
                        continue;
                }
                else {
                    printf("\n\nServer : packet no. %d -> BAD", protocol.seq);
                    continue;
                }
                if (protocol.flags.name || !(protocol.type.text || protocol.type.binary))
                    continue;
            }
            else
            {
                if (reply(host, host, buffer))
                    printf("\n\nServer : packet no. %d -> OK", protocol.seq);
                else
                    continue;

            }

            Message mesg = Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq);
            if (protocol.stream == lastStream)
            {
                recent->addFragment(mesg);
            }
            else {

                recent = findStream(streams, protocol.stream);
                recent->addFragment(mesg);
                lastStream = protocol.stream;
            }

            
          
            if (protocol.flags.quit || (recent && recent->finished)) {
                //** prijmeme posledny paket komunikacie **//

                //////// toto oprav na zrozumitelnejsie ////////
                recent->initializeMissing(protocol.seq);
                
                if (checkCompletition(streams, protocol.stream)) {
                
                    std::cout << "transfer done" << std::endl;
                if (protocol.type.binary) {
                    fileName = new char[filenameSize + 1];
                               
                    concat(namevec, protocol.stream, fileName);
                    fileName[filenameSize] = '\0';
                               
                    int size = concat(streams, protocol.stream, filebuffer);
                                

                    std::cout << "saving" << std::endl;
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

                }
                else {      
                  
                    continue;
                     
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