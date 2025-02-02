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

#define MAX_FILE 2500000
#pragma comment(lib,"ws2_32.lib")

/// <summary>
/// Funkcia podla prijatych dat skontroluje CRC a vyhotovi opdoved ACK alebo RETRY ak boli data poskodene
/// </summary>
/// <returns>
///     True ak data prisli neposkodene inak false
/// </returns>
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

    //prijal keepalive spravu
    if (protocol.type.keep_alive) {
        protocol.flags.ack = 1;
        sendto(client, arq(protocol, -1), HEADER_8, 0, (sockaddr*)&host, sizeof(host));
        return true;
    }

    ZeroMemory(&protocol, sizeof(protocol));
    protocol.type.len = size;


    //crc sed�
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

/// <summary>
/// 
/// </summary>
/// <param name="listenSocket">Socket, na ktory prijima spravy</param>
/// <param name="socketi">
///     Obsahuje IP adresu servera (127.0.0.1) a cislo portu, na kotrom prijima spravy 
/// </param>
void recieve(SOCKET listenSocket, struct sockaddr_in socketi) {

    char* buffer = new char [MAX_FRAG];
    int slen ;
    char* filebuffer = new char[MAX_FILE];
     
    int c = 0;

    sockaddr_in host;
    slen = sizeof(host);
    header protocol, reference;
    message transfered;
  
    Stream* recent = nullptr;
       
    char* fileName = nullptr;
    short filenameSize = 0, lastStream = -1;

    message* ptr = &transfered;
    std::vector<Stream> namevec;
    std::vector<Stream> streams; 
 
    //  cyklus prij�ma spr�vy dokial pou�ivate� neukon�� spojenie
    while (true) {
        
        if ((recvfrom(listenSocket, (char*)buffer, MAX_FRAG, 0, (struct sockaddr*)&host, &slen)) == SOCKET_ERROR) {

            printf("Server : connection lost # %d", WSAGetLastError());
            return;
        }
        else {

            printf("\nReceived packet from %s:%d\n", inet_ntoa(host.sin_addr), ntohs(host.sin_port));

            analyzeHeader(protocol, buffer);

            if (protocol.type.control) {

                if (protocol.flags.init) {
                    ptr = &transfered;
                    reference = protocol;
                    Stream incoming = Stream(reference.stream, protocol.stream);
                    Stream* str;
                        
                        //definuje ci ide o tok s datami suboru alebo s nazvami suboru
                        if (protocol.flags.name && !protocol.flags.resesend) {
                            namevec.clear();
                            namevec.push_back(incoming);
                            
                        }
                        else if (!protocol.flags.resesend) {
                            streams.clear();
                            streams.push_back(incoming);
                        }

                }
                if (protocol.type.keep_alive) {
                    protocol.seq = 0;
                    protocol.type.len = 2;
                }

                //odpoveda na prijaty fragment
                if (reply(host, host, buffer)) {

                    printf("Server : packet no. %d -> OK (%d B)\n\n", protocol.seq, protocol.data_len + protocol.type.len*4);

                    if (protocol.type.binary && protocol.flags.name) {
                        Stream* str;

                        str = findStream(namevec, protocol.stream);

                        //vytvorime fragment
                        Message mesg = Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq);
                        //vlozime ho do pola
                        str->addFragment(mesg);

                        //dlzka nazvu suboru
                        filenameSize += protocol.data_len;

                        if (protocol.flags.quit) {
                            str->initializeMissing(protocol.seq);
                            //ak bol prijaty posledny fragment suboru, tak skontroluje ci prijal vsetky fragmenty s mensim sekvencnym cislom ako posledny fragment
                            if (checkCompletition(namevec, protocol.stream)) {
                                str->finished = 1;
                            }
                        }
                    }
                    else if (protocol.type.keep_alive)
                        continue;
                }
                else {
                    //bol prijaty fragment s chybou
                    printf("Server : packet no. %d ------> BAD (%d B)\n\n", protocol.seq, protocol.data_len + protocol.type.len * 4);
                    continue;
                }
                if (protocol.flags.name || !(protocol.type.text || protocol.type.binary))
                    continue;
            }
            else
            {   //odpoveda na fragmenty, ktore nemali control bit rovny 1
                if (reply(host, host, buffer))
                    printf("Server : packet no. %d -> OK (%d B)\n\n", protocol.seq, protocol.data_len + protocol.type.len * 4);
                else {
                    printf("Server : packet no. %d ------> BAD (%d B)\n\n", protocol.seq, protocol.data_len + protocol.type.len * 4);
                    continue;
                }
                    

            }
            Message mesg = Message(buffer + protocol.type.len * 4, protocol.data_len, protocol.seq);
            
            if (protocol.flags.name) {
                Stream* str;
                str = findStream(namevec, protocol.stream);
                str->addFragment(mesg);
                filenameSize += protocol.data_len;
                continue;
            }


            //ak sa jedna rovnaky tok dat ako predosly fragment tak preskocime vyhladavanie toku
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

                recent->initializeMissing(protocol.seq);
                
                if (checkCompletition(streams, protocol.stream)) {
                
                    std::cout << "transfer done" << std::endl;
                if (protocol.type.binary) {
                    fileName = new char[MAX_PATH];
                    ZeroMemory(fileName, MAX_PATH);
                    
                    concat(namevec, protocol.stream, fileName);
                    fileName[filenameSize] = '\0';
                    char filepath[MAX_PATH] = { 0 };

                    int size = concat(streams, protocol.stream, filebuffer);
                    
                    //vytvorenie suboru

                    saveFileTo(fileName, filepath);

                    printf("Saved to : %s", filepath);
                    std::ofstream file(filepath, std::ios::out | std::ios::binary );
                    
                    if(file){
                        file.write(filebuffer, size);
                        file.close();
                    }
                    else
                        printf("ERROR : couldnt save file");

                    delete[] fileName;
                                
                }
                //ak sme prijali textovu spravu
                if (protocol.type.text) {
                    ZeroMemory(buffer, 512);
                    concat(streams, protocol.stream, buffer);
                    std::cout << "Incomming message :";
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

        printf("DEBUG : The Winsock dll found!\n");

        printf("DEBUG : The current status is: %s.\n", wsaData.szSystemStatus);

    }



    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)

    {

        printf("DEBUG : The dll do not support the Winsock version %u.%u!\n",

            LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));


        WSACleanup();


        return;

    }

    else

    {

        printf("DEBUG : The dll supports the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion),

            HIBYTE(wsaData.wVersion));

        printf("DEBUG : The highest version this dll can support: %u.%u\n", LOBYTE(wsaData.wHighVersion),

            HIBYTE(wsaData.wHighVersion));
        printf("==========================================================================\n");
        printf("==================================SERVER==================================\n");
        printf("Stlac [0] pre vypnutie , [h] help\n\n");
        return;
    }
}

/// <summary>
///     Spustenie prijimaca
/// </summary>
void Reciever::run() {

    SOCKET listenSocket = INVALID_SOCKET;
    SOCKET clientSocket = INVALID_SOCKET;

    int slen, recieved_len;

    struct sockaddr_in host;
    slen = sizeof(host);
    listenSocket = socket(socketi.sin_family, SOCK_DGRAM, IPPROTO_UDP);
    if (listenSocket == INVALID_SOCKET) {
        std::cout << "ERROR : Bad socket init." << std::endl;
        return;
    }
    
    int iResult = bind(listenSocket, (struct sockaddr*)&socketi, sizeof(socketi));
    if (iResult == SOCKET_ERROR) {
        std::cout << "ERROR : bind error !!" << std::endl;
        return;
    }
    
    std::cout << "Server : Waiting for transfer..." << std::endl;
    std::thread recieving(&recieve, listenSocket, host);
    
    
    while (alive) {
        int input;
        input = getchar();

        switch (input) {
            case '0':
                alive = false;
                break;
            case 'h':
                printf("Stlac [0] pre vypnutie , [h] help");
                break;
            default:
                continue;
        }
    }
    
    if (recieving.joinable())
        closesocket(listenSocket);
        recieving.join();

        return;
}

void Reciever::cleanup() {

    if (WSACleanup() == SOCKET_ERROR)
    {
        std::cout << "Error : WSACleanup failed " + WSAGetLastError();
    }

}