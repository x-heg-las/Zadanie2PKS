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
#include <map>
#include <future>
#include <chrono>
#include <assert.h>

#include "include/checksum.h"


std::mutex free_to_send_mtx;
std::atomic<bool> ready;

void keepAlive(sockaddr_in hostsockaddr, SOCKET conectionsocket) {

    
    struct fragment ka_message;
    ZeroMemory(&ka_message, sizeof(ka_message));
    ka_message.header.dataLength = 0;
    ka_message.header.type.keep_alive = 1;
    char message[25] = { 0 };
    char recvbuf[25] = { 0 };
    int result;

    BOOL bOptVal = FALSE;
    int bOptLen = sizeof(BOOL);

    sockaddr_in addr_in;
    int slen = sizeof(addr_in);
    int iOptVal = 15000;
    int iOptLen = sizeof(int);

    setsockopt(conectionsocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iOptVal, iOptLen);
    
    copyHeader(message, ka_message.header);
    
    while (ready.load()) {


        if (result = sendto(conectionsocket, message, HEADER_8, 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

            std::cout << "Client : Failed to send data" << std::endl;
        }

        if ((recvfrom(conectionsocket, (char*)recvbuf, 25, 0, (struct sockaddr*)&addr_in, &slen)) != SOCKET_ERROR){}

    }




}

void recieveMessage(SOCKET listenSocket, int type, std::map<int, int> &seq){
    
    char buffer[512] = { 0 };
    sockaddr_in addr_in;
    int slen = sizeof(addr_in);
    header protocol;
    ready.store(true);
    int result = SOCKET_ERROR;

    BOOL bOptVal = FALSE;
    int bOptLen = sizeof(BOOL);

    int iOptVal = 1000;
    int iOptLen = sizeof(int);
    setsockopt(listenSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iOptVal, iOptLen);
    ZeroMemory(buffer, 512);
  
    while (ready.load()) {

        
      
        if ( (recvfrom(listenSocket, (char*)buffer, 512, 0, (struct sockaddr*)&addr_in, &slen)) != SOCKET_ERROR) {
            
            analyzeHeader(protocol, buffer);

            if (type == ACK && protocol.flags.ack) {

                free_to_send_mtx.lock();
                seq.erase(protocol.seq);
                free_to_send_mtx.unlock();
            }
            

        }
    }

}
//oprav arq
int Sender::connect(std::string filePath, int fragmentLength, sockaddr_in host, SOCKET socket) {

    struct fragment stream;
   
  
    ZeroMemory(&stream, sizeof(stream));
    char* fileBuffer = new char[ MAX_PATH];
    ZeroMemory(fileBuffer,  MAX_PATH);
    int result, frg_sent = 0 ;

    std::string fileName = filePath.substr(filePath.find_last_of("/\\") + 1);

    memcpy(fileBuffer, fileName.c_str(), fileName.length() + 1);

    if (fileName.length() + 1 > fragmentLength) {
        stream.header.type.len = 3;
    }
    else
        stream.header.type.len = 2;

    std::vector<struct fragment> data;
    std::map<int, int> unrecieved;

    fragmentMessage(data, stream, fileName.length(), fileBuffer, fragmentLength, NAME);

   std::thread listening(recieveMessage, socket, ACK, std::ref(unrecieved));


    for(struct fragment &msg : data) {

        unrecieved[msg.header.sequenceNumber] = msg.header.sequenceNumber;
        if (result = sendto(socket, msg.data, (msg.header.dataLength + msg.header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

            std::cout << "Client : Failed to send data" << std::endl;
        }
        else {
            frg_sent++;
        }
        if (!(frg_sent%120) && !unrecieved.empty()) {//window
            // toto sa da este upraavit
            
            for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

                //modifikacia dat - nastavenie retry flagu
                *((unsigned char*)data.at(it->first).data) |= (1);
                //nove crc pre zmenene data
                unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
                //zmena crc v hlavicke so zmenenym flagom
                *((unsigned short*)data.at(it->first).data + 1) = newCrc;

                if (result = sendto(socket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

                    std::cout << "Client : Failed to send data" << std::endl;
                }
            }
        }

    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    if (!unrecieved.empty()) {

        for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {
            if (result = sendto(socket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

                std::cout << "Client : Failed to send data" << std::endl;
            }
        }
    }

    ready.store(false);
    

    listening.join();


    return 0;
}  // oprav arq

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
    std::vector<struct fragment> data;

    int fragmentCount = fragmentMessage(data, stream, fileSize, fileBuffer, fragmentLen, FILE);
    std::map<int, int> unrecieved;

    std::thread listening(&recieveMessage, connectionSocket, ACK,std::ref(unrecieved));
    
    char change;
    int frg_sent = 0;
    for(struct fragment &msg : data){
        unrecieved[msg.header.sequenceNumber] = msg.header.sequenceNumber;

        
        if (msg.header.sequenceNumber%70 == 25)
        {
            change = *(msg.data + 18);
            *(msg.data+ 18) = '#';
        }


        


        if (result = sendto(connectionSocket, msg.data, (msg.header.dataLength + msg.header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {
        
            std::cout << "Client : Failed to send data" << std::endl;

        }
        else {
            frg_sent++;
        }
       

        if (msg.header.sequenceNumber % 70 == 25) {
            *(msg.data + 18) = change;


        }
        //std::this_thread::sleep_for(std::chrono::microseconds(250));
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
        for(int i = 0; i < 2 && !(frg_sent%WINDOW); i++){

            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // pockame chvilu na oneskorene pakety

            if (!unrecieved.empty()) {//window  // potom skus ten mutex co spravi :)

                free_to_send_mtx.lock();
                for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

                    //modifikacia dat - nastavenie retry flagu
                    *((unsigned char*)data.at(it->first).data) |= (1);
                    //nove crc pre zmenene data
                    unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
                    //zmena crc v hlavicke so zmenenym flagom
                    *((unsigned short*)data.at(it->first).data + 1) = newCrc;

                    if (result = sendto(connectionSocket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

                        std::cout << "Client : Failed to send data" << std::endl;
                    }
                }
                free_to_send_mtx.unlock();
            }
        }
                
        
    }
  
    
    if (!unrecieved.empty()) {
        free_to_send_mtx.lock();
        for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {
            
            //modifikacia dat - nastavenie retry flagu
            *((unsigned char*)data.at(it->first).data) |= (1);
            //nove crc pre zmenene data
            unsigned short newCrc= crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
            //zmena crc v hlavicke so zmenenym flagom
            *((unsigned short*)data.at(it->first).data + 1) = newCrc;


            if (result = sendto(connectionSocket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

                std::cout << "Client : Failed to send data" << std::endl;
            }
        }
        free_to_send_mtx.unlock();
    }

    ready.store(false);


    listening.join();
    printf("POSLANYCH :  %d \n", frg_sent);
    // este neak uvolnovanie dat zabezpeciit 
    return 1;
}

int Sender::sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket, int type) // type nemusi byt
{
    int len = message.length();
    int result;
    struct fragment stream;
    ZeroMemory(&stream, sizeof(stream));

    std::vector<struct fragment> data;

  //  if ((len+1) + HEADER_8 > fragmentLen) {

        stream.header.flags.fragmented = 1;
        stream.header.type.len = 3;
        stream.header.type.text = 1;
        
  

        fragmentMessage(data ,stream, len+1, (char*)message.c_str(), fragmentLen, TEXTM);
        int cnt = 0;
        for(auto &msg : data) {
            
            if (result = sendto(connectionSocket, msg.data, (msg.header.dataLength + msg.header.type.len*4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {
                
                std::cout << "Client :fragment send error" << std::endl;

            }
            cnt++;
        }
        printf("POSLANYCH : _%d", cnt);
        
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
    sockaddr_in host;

    host.sin_family = AF_INET;
    host.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    
    connectionSocket = socket(hints.ai_family, hints.ai_socktype, IPPROTO_UDP);

    int iResult = bind(connectionSocket, (struct sockaddr*)&host, sizeof(host));
    if (iResult == SOCKET_ERROR) {
        std::cout << "ERROR : bind error !!" << std::endl;
    }
    
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

        std::thread keepAlive_t;

        switch (choice) {
            case 't':
                ready.store(false);
                std::cout << "Message : ";
                std::getline(std::cin, msg);
                if(msg.size() > 0)
                    _resullt = sendMessage(msg, fragment, hostsockaddr, connectionSocket, TEXTM);

                keepAlive_t = std::thread(&keepAlive, hostsockaddr, connectionSocket);
                break;
            case 'f' :
                ready.store(false);

                filename = getFilename();
                
                _resullt = connect(filename, fragment, hostsockaddr, connectionSocket);

                _resullt = sendFile(filename, fragment, hostsockaddr, connectionSocket);


                keepAlive_t = std::thread(&keepAlive, hostsockaddr, connectionSocket);
                break;
            case 'l':
                std::cin >> fragment;
                break;
            case 'e':
                closesocket(connectionSocket);
                std::cout << "Client : client is shutting downn\nReturning to main screen..." << std::endl;
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
