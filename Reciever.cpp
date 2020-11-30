#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "Reciever.h"
#include "InitControll.h"
#include "Protocol.h"
#include <WinSock2.h>
#include <Windows.h>
#include <thread>

#include <vector>
#include <iostream>

#pragma comment(lib,"ws2_32.lib")

void reply(sockaddr_in host, sockaddr_in server, char* data) {

    header protocol;
    analyzeHeader(protocol, data);

    unsigned short sum = crc(data);
    SOCKET client = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);


    if (!sum) {
        ZeroMemory(&protocol, sizeof(protocol));
        protocol.flags.ack = 1;
        sendto(client, arq(protocol, protocol.data_len), HEADER_8, 0, (sockaddr*)&host, sizeof(host));
    }
    else {
        ZeroMemory(&protocol, sizeof(protocol));
        protocol.flags.retry = 1;
        sendto(client, arq(protocol, protocol.data_len), HEADER_8, 0, (sockaddr*)&host, sizeof(host));
    }
}

void recieve(SOCKET listenSocket, struct sockaddr_in socketi) {

    char buffer[512];
    int slen ;

     
    int c = 0;

    sockaddr_in host;
    slen = sizeof(host);
    header protocol, reference;
    message transfered;
    stream* dataFlow;

    message* ptr = &transfered;
   // stream* streamptr = dataFlow;
    std::vector<Stream> streams; 
   /// std::vector<Stream>::iterator it;
    ZeroMemory(&dataFlow, sizeof(dataFlow));
    while (true) {

        ZeroMemory(&buffer, sizeof(512));

        if ((recvfrom(listenSocket, (char*)buffer, 512, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            std::cout << "Server : connection lost";
            return;
        }
        else {
            
            reply(host, host, buffer);
            printf("Received packet from %s:%d\n", inet_ntoa(host.sin_addr), ntohs(host.sin_port));

            analyzeHeader(protocol, buffer);

            if (protocol.flags.init) {
                ptr = &transfered;
                reference = protocol;
                Stream incoming = Stream(reference.stream, 0);
                streams.push_back(incoming);
            }


            if (!protocol.flags.quit) {
                

               
                //ptr->data = new char[protocol.data_len];
                //memcpy(ptr->data, buffer + protocol.type.len * 4, protocol.data_len);
                //ptr->offset = protocol.seq;
                //ptr->stream = protocol.stream;




                if (protocol.flags.name) {
                    int namelen = strlen(ptr->data);
                  //  ptr->data += namelen + 1;
                    //ptr->len = protocol.data_len - (namelen + 1);
                   // Message fragment = Message(buffer + protocol.type.len * 4, protocol.data_len - (namelen + 1), protocol.seq);
                    Stream* str;

                  //  fragment.data += namelen + 1;
                    std::find_if(streams.begin(),
                                 streams.end(),
                                 [&var = protocol.stream,&se = str]
                    (Stream& s) -> bool { if (s.streamnumber == var) { se = &s; return true; }return false;  });
                    
                    str->addFragment(Message(buffer + protocol.type.len * 4 + namelen + 1, protocol.data_len - (namelen + 1), protocol.seq));
                    
                    
                }
                else {
                   // Message fragment = Message(buffer + protocol.type.len * 4, protocol.data_len , protocol.seq);
                    //ptr->len = protocol.data_len;
                    Stream* str;
                    std::find_if(streams.begin(),
                                 streams.end(),
                        [&var = protocol.stream,  &se = str]
                    (Stream& s) -> bool { if (s.streamnumber == var) { se = &s; return true; }return false;  });

                    str->addFragment(Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq));

                }
                
            }
            else { // toto sa da upravit na menej kodu 

                if (protocol.flags.name) {
                    int namelen = strlen(ptr->data);
                    //  ptr->data += namelen + 1;
                      //ptr->len = protocol.data_len - (namelen + 1);
                   // Message fragment = Message(buffer + protocol.type.len * 4, protocol.data_len - (namelen + 1), protocol.seq);
                    Stream* str;
                   // fragment.data += namelen + 1;
                    std::find_if(streams.begin(),
                        streams.end(),
                        [&var = protocol.stream, &se = str]
                    (Stream& s) -> bool { if (s.streamnumber == var) { se = &s; return true; }return false;  });

                    str->addFragment(Message(buffer + protocol.type.len * 4 + (namelen + 1), protocol.data_len - (namelen + 1), protocol.seq));


                }
                else {
                   // Message fragment = Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq);
                    //ptr->len = protocol.data_len;
                    Stream* str;
                    std::find_if(streams.begin(),
                        streams.end(),
                        [&var = protocol.stream, &se = str]
                    (Stream& s) -> bool { if (s.streamnumber == var) { se = &s; return true; }return false;  });

                    str->addFragment(Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq));
                }


                //////// toto oprav na zrozumitelnejsie ////////


                if (reference.type.text) {

                    ZeroMemory(buffer, 512);
                    concat(streams, protocol.stream, buffer);
                    std::cout << buffer << std::endl;
                                 
                    
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

    char buffer[512];
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