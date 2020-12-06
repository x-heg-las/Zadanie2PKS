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

#include "include/checksum.h" // netreba


std::mutex free_to_send_mtx;
std::atomic<bool> ready;

/// <summary>
/// Keepalive funkcia je spustena vzdy po preneseni subou pre udrzanie spojenia medzi klientom a serverom.
/// Keepalive spravy su posielane kazdych 15 sekund. Ak server neodpovie 3x po sebe, tak sa spojenie uzavrie.
/// </summary>
void keepAlive(sockaddr_in hostsockaddr, SOCKET conectionsocket) {

    struct fragment ka_message;
    ZeroMemory(&ka_message, sizeof(ka_message));

    //inicializacia hlavicky spravy
    ka_message.header.dataLength = 0;
    ka_message.header.type.control = 1;
    ka_message.header.type.keep_alive = 1;
    char message[25] = { 0 };
    char recvbuf[25] = { 0 };
    int result;
    char timeouts = 0;
    header protocol;

    ready.store(true);

    sockaddr_in addr_in;
    int slen = sizeof(addr_in);
    int iOptVal = 50;
    int iOptLen = sizeof(int);

    //nastavenie timeoutu na prijatie paketu
    setsockopt(conectionsocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iOptVal, iOptLen);
    
    copyHeader(message, ka_message.header);
    
    while (ready.load()) {

        //cakaj 15 sekund pred odoslanim spravy
        std::this_thread::sleep_for(std::chrono::milliseconds(15000));
        if (result = sendto(conectionsocket, message, HEADER_8, 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

            std::cout << "Client : Connetion lost!... connection closed\n" << std::endl;
            return;
        }

        if ((recvfrom(conectionsocket, (char*)recvbuf, 25, 0, (struct sockaddr*)&addr_in, &slen)) != SOCKET_ERROR){
        
            analyzeHeader(protocol, recvbuf);
            
            if (protocol.flags.ack) {

                printf("Client : Connected to %s\n\n", inet_ntoa(addr_in.sin_addr));
                timeouts = 0;
                continue;
            }
        }

        //ak neprijmeme potvrdenie
        timeouts++;
        if (timeouts > 3) {
            printf("Client : Connection lost!\n\n");
            ready.store(false);
            return;
        }
    }
}

/// <summary>
/// Funkcia pre prijimanie sprav(odpovedi) od servera. 
/// Odpovedami su ACK a RETRY spravy
/// </summary>
void recieveMessage(SOCKET listenSocket, int type, std::map<int, int> &seq){
    
    char buffer[128] = { 0 };
    sockaddr_in addr_in;
    int slen = sizeof(addr_in);
    header protocol;

    ready.store(true);
    int result = SOCKET_ERROR;

    int iOptVal = 1000; // timeut na prijatie spravy 500 ms
    int iOptLen = sizeof(int);

    //upravenie timeoutu pre prijatie sprav od servera
    setsockopt(listenSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&iOptVal, iOptLen);
    ZeroMemory(buffer, 128);
  
    //cyklus iteruje dokial je premenna ready nastavena na true
    while (ready.load()) {

        if ((recvfrom(listenSocket, (char*)buffer, 128, 0, (struct sockaddr*)&addr_in, &slen)) != SOCKET_ERROR) {
            
            analyzeHeader(protocol, buffer);

            if (type == ACK && protocol.flags.ack) {

                free_to_send_mtx.lock();
                seq.erase(protocol.seq);
                free_to_send_mtx.unlock();
            }          
        }
    }
}

/// <summary>
///     Funkcia pre nadviazanie spojenia so serverom. Tato funkcia je volana len pri posielani suboru.
///     Pocas nadviazania komunikacie sa posiela nazov suboru, ktory posielame.
/// </summary>
/// <param name="filePath">
///     Absolutna cesta k suboru, ktory sa posiela.
/// </param>
/// <param name="fragmentLength">
///     Nastavena max. velkost fragmentu.
/// </param>
/// <param name="host"></param>
/// <param name="socket"></param>
/// <returns>
///     Pocet odoslanych fragmentov. Ak sa nepodarilo nadviazat spojenie vrati -1.
/// </returns>
int Sender::connect(std::string filePath, int fragmentLength, sockaddr_in host, SOCKET socket) {

    struct fragment stream;
     
    ZeroMemory(&stream, sizeof(stream));
    char fileBuffer[MAX_PATH] = { 0 };
    int result, frg_sent = 0 ;

    //vyrezanie sazvu suboru z absolutnej cesty
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

    std::thread listening(&recieveMessage, socket, ACK, std::ref(unrecieved));

    for(struct fragment &msg : data) {

        unrecieved[msg.header.sequenceNumber] = msg.header.sequenceNumber;

        if (result = sendto(socket, msg.data, (msg.header.dataLength + msg.header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

            std::cout << "Client : Failed to send data... connection closed" << std::endl;
            ready.store(false);

            if (listening.joinable())
                listening.join();
            freeData(data);
           
            return -1;
        }
        else {
            printf("Cient : sending packet... size:%d B (header %d B)\n\n", msg.header.dataLength + msg.header.type.len * 4, msg.header.type.len * 4);
            frg_sent++;
        }

        if (!(frg_sent % WINDOW)) {
            int resend = 0;

            while (!unrecieved.empty()) {

                if (resend == 2){
                    printf("Client : No response .. connection closed");
                    ready.store(false);

                    if (listening.joinable())
                        listening.join();
                    freeData(data);
                    return -1;
                }

                resend++;
                free_to_send_mtx.lock();
                for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

                    //modifikacia dat - nastavenie retry flagu
                    *((unsigned char*)data.at(it->first).data) |= (1);
                    //nove crc pre zmenene data
                    unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
                    //zmena crc v hlavicke so zmenenym flagom
                    *((unsigned short*)data.at(it->first).data + 1) = newCrc;

                    if (result = sendto(socket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

                        std::cout << "Client : Failed to send data... connection closed" << std::endl;
                        ready.store(false);

                        if (listening.joinable())
                            listening.join();
                        free_to_send_mtx.unlock();
                        freeData(data);
                        return -1;
                    }
                    else {
                        printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                        frg_sent++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                free_to_send_mtx.unlock();

                //pocka na potvrdzovacie spravy
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
    }

  
    int resend = 0;
    while (!unrecieved.empty()) {
        
        if (resend == 2) {
            printf("Client : No response .. connection closed");
            ready.store(false);

            if (listening.joinable())
                listening.join();
            freeData(data);
            return -1;
        }
        resend++;
        free_to_send_mtx.lock();
        for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

            //modifikacia dat - nastavenie retry flagu
            *((unsigned char*)data.at(it->first).data) |= (1);
            //nove crc pre zmenene data
            unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
            //zmena crc v hlavicke so zmenenym flagom
            *((unsigned short*)data.at(it->first).data + 1) = newCrc;

            if (result = sendto(socket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&host, sizeof(host)) == SOCKET_ERROR) {

                std::cout << "Client : Failed to send data... connection closed" << std::endl;
        
                ready.store(false);

                if (listening.joinable())
                    listening.join();
                free_to_send_mtx.unlock();
                freeData(data);
                return -1;
            }
            else {
                printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                frg_sent++;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(3));
        }
        free_to_send_mtx.unlock();
        //pocka na potvrdzovacie spravy
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    ready.store(false);
    if(listening.joinable())
        listening.join();
    freeData(data);
    return frg_sent;
} 

int Sender::sendFile(std::string filePath, int fragmentLen, sockaddr_in hostsockaddr, SOCKET connectionSocket, int errPacket)
{

    std::ifstream fileInput(filePath, std::ifstream::in | std::ifstream::binary);
    
    struct fragment stream;

    //Zistenie velkosti suboru
    fileInput.seekg(0, fileInput.end);
    int fileSize = fileInput.tellg() , result = SOCKET_ERROR;
    fileInput.seekg(0, fileInput.beg);

    ZeroMemory(&stream, sizeof(stream));
    char* fileBuffer = new char[fileSize + MAX_PATH];
    ZeroMemory(fileBuffer, fileSize + MAX_PATH);
   
    //zistenie velkosti potrebnej hlavicky a jej nasledne inicializovanie
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

        //Poskodenie dat datagramu
        if (errPacket && msg.header.sequenceNumber && msg.header.sequenceNumber % errPacket)
        {
            change = *(msg.data + msg.header.type.len*4 + 3);
            *(msg.data + msg.header.type.len * 4 + 3) += 1;
        }


        if (result = sendto(connectionSocket, msg.data, (msg.header.dataLength + msg.header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {
        
            std::cout << "Client : Failed to send data" << std::endl;
            ready.store(false);

            if (listening.joinable())
                listening.join();
            freeData(data);
            return -1;

        }
        else {
            printf("Cient : sending packet... size:%d B (header %d B)\n\n", msg.header.dataLength + msg.header.type.len * 4, msg.header.type.len * 4);
            frg_sent++;
        }
       
        //Naprava poskodenej casti datagramu
        if (errPacket && msg.header.sequenceNumber && msg.header.sequenceNumber % errPacket) {
            *(msg.data + msg.header.type.len * 4 + 3) = change;
        }

        //std::this_thread::sleep_for(std::chrono::microseconds(250));
        std::this_thread::sleep_for(std::chrono::milliseconds(3));
       
        if (!(frg_sent % WINDOW)) {// ak sme poslali zadany pocet fragmetov
            int resend = 0;

            while (!unrecieved.empty()) {

                if (resend == 2) {
                    printf("Client : No response .. connection closed");
                    ready.store(false);

                    if (listening.joinable())
                        listening.join();
                    freeData(data);
                    return -1;
                }

                resend++;
                free_to_send_mtx.lock();
                for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

                    //modifikacia dat - nastavenie retry flagu
                    *((unsigned char*)data.at(it->first).data) |= (1);
                    //nove crc pre zmenene data
                    unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
                    //zmena crc v hlavicke so zmenenym flagom
                    *((unsigned short*)data.at(it->first).data + 1) = newCrc;

                    if (result = sendto(connectionSocket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

                        std::cout << "Client : Failed to send data... connection closed" << std::endl;
                        ready.store(false);

                        if (listening.joinable())
                            listening.join();
                        free_to_send_mtx.unlock();
                        freeData(data);
                        return -1;
                    }
                    else {
                        printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                        frg_sent++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                free_to_send_mtx.unlock();

                //pocka na potvrdzovacie spravy
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
    }
  
    int resend = 0; 
    while (!unrecieved.empty()) {

        if (resend == 2) {
            printf("Client : No response .. connection closed");

            ready.store(false);
            if (listening.joinable())
                listening.join();
            freeData(data);
            return -1;
        }

        resend++;

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
               

                ready.store(false);

                if (listening.joinable())
                    listening.join();
                free_to_send_mtx.unlock();
                freeData(data);
                return -1;
            }
            else {
                printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                frg_sent++;
            }
        }
        free_to_send_mtx.unlock();

        //pocka na potvrdzovacie spravy
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    ready.store(false);

    if(listening.joinable())
        listening.join();

    
    delete[] fileBuffer;
    freeData(data);
    return frg_sent;
}

int Sender::sendMessage(std::string message, int fragmentLen, struct sockaddr_in  hostsockaddr, SOCKET connectionSocket, int errPacket) 
{
    int len = message.length();
    int result,frg_sent = 0;
    struct fragment stream;
    ZeroMemory(&stream, sizeof(stream));
    char change;
    std::map<int, int> unrecieved;
    std::vector<struct fragment> data;

    //zistenie velkosti potrebnej hlavicky a jej nasledne inicializovanie
    if (len+1 + HEADER_8 > fragmentLen) {

        stream.header.flags.fragmented = 1;
        stream.header.type.len = 3;
        stream.header.type.text = 1;
    }
    else {
        stream.header.type.len = 2;
        stream.header.type.text = 1;
    }
    
    fragmentMessage(data, stream, len + 1, (char*)message.c_str(), fragmentLen, TEXTM); // len+1 aby sa odoslal aj '\0' znak
    std::thread listening(&recieveMessage, connectionSocket, ACK, std::ref(unrecieved));

    for (struct fragment& msg : data) {
        
        
       
        unrecieved[msg.header.sequenceNumber] = msg.header.sequenceNumber;

        //Poskodenie dat datagramu
        if (errPacket && msg.header.sequenceNumber && msg.header.sequenceNumber % errPacket)
        {
            change = *(msg.data + msg.header.type.len * 4 + 3);
            *(msg.data + msg.header.type.len * 4 + 3) += 1;
        }


        if (result = sendto(connectionSocket, msg.data, (msg.header.dataLength + msg.header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

            std::cout << "Client : Failed to send data" << std::endl;

            ready.store(false);
            if (listening.joinable()) {
                listening.join();
            }
            freeData(data);
            return -1;

        }
        else {
            printf("Cient : sending packet... size:%d B (header %d B)\n\n", msg.header.dataLength + msg.header.type.len * 4, msg.header.type.len * 4);
            frg_sent++;
        }

        //Naprava poskodenej casti datagramu
        if (errPacket && msg.header.sequenceNumber && msg.header.sequenceNumber % errPacket) {
            *(msg.data + msg.header.type.len * 4 + 3) = change;
        }

        //std::this_thread::sleep_for(std::chrono::microseconds(250));
        std::this_thread::sleep_for(std::chrono::milliseconds(3));

        if (!(frg_sent % WINDOW)) {// ak sme poslali zadany pocet fragmetov
            int resend = 0;

            while (!unrecieved.empty()) {

                if (resend == 2) {
                    printf("Client : No response .. connection closed");
                    ready.store(false);
                    if (listening.joinable()) {
                        listening.join();
                    }
                    freeData(data);
                    return -1;
                }

                resend++;
                free_to_send_mtx.lock();
                for (std::map<int, int>::iterator it = unrecieved.begin(); it != unrecieved.end(); ++it) {

                    //modifikacia dat - nastavenie retry flagu
                    *((unsigned char*)data.at(it->first).data) |= (1);
                    //nove crc pre zmenene data
                    unsigned short newCrc = crc(data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4));
                    //zmena crc v hlavicke so zmenenym flagom
                    *((unsigned short*)data.at(it->first).data + 1) = newCrc;

                    if (result = sendto(connectionSocket, data.at(it->first).data, (data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4), 0, (struct sockaddr*)&hostsockaddr, sizeof(hostsockaddr)) == SOCKET_ERROR) {

                        std::cout << "Client : Failed to send data... connection closed" << std::endl;
                        ready.store(false);
                        if (listening.joinable()) {
                            listening.join();
                        }
                        free_to_send_mtx.unlock();
                        freeData(data);
                        return -1;
                    }
                    else {
                        printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                        frg_sent++;
                    }

                    std::this_thread::sleep_for(std::chrono::milliseconds(3));
                }
                free_to_send_mtx.unlock();

                //pocka na potvrdzovacie spravy
                std::this_thread::sleep_for(std::chrono::milliseconds(250));
            }
        }
    }

    int resend = 0;
    while (!unrecieved.empty()) {

        if (resend == 2) {
            printf("Client : No response .. connection closed");
            ready.store(false);
            if (listening.joinable()) {
                listening.join();
            }
            freeData(data);
            return -1;
        }

        resend++;

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
                ready.store(false);
                if (listening.joinable()) {
                    listening.join();
                }
                freeData(data);
                free_to_send_mtx.unlock();
                return -1;
            }
            else {
                printf("Cient : sending packet... size:%d B (header %d B)\n\n", data.at(it->first).header.dataLength + data.at(it->first).header.type.len * 4, data.at(it->first).header.type.len * 4);
                frg_sent++;
            }
        }
        free_to_send_mtx.unlock();

        //pocka na potvrdzovacie spravy
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    ready.store(false);
    if (listening.joinable()) {
        listening.join();
    }

    freeData(data);
    return frg_sent;
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

        printf("DEBUG : The Winsock dll found!\n");

        printf("DEBUG : The current status is: %s.\n", wsaData.szSystemStatus);

    }

    if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2) 
    {

        //Tell the user that we could not find a usable WinSock DLL

        printf("DEBUG : The dll do not support the Winsock version %u.%u!\n",

            LOBYTE(wsaData.wVersion), HIBYTE(wsaData.wVersion));

        // When your application is finished call WSACleanup

        WSACleanup();

        // and exit

        return ;

    }

    else

    {

        printf("DEBUG : The dll supports the Winsock version %u.%u!\n", LOBYTE(wsaData.wVersion),

            HIBYTE(wsaData.wVersion));

        printf("DEBUG : The highest version this dll can support: %u.%u\n", LOBYTE(wsaData.wHighVersion),

            HIBYTE(wsaData.wHighVersion));

        return ;

    }
}

void Sender::run()
{
    int _resullt = 0;

    ZeroMemory(&hints, sizeof(hints));

    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_protocol = IPPROTO_UDP;

    SOCKET connectionSocket = INVALID_SOCKET;
    sockaddr_in host;

    host.sin_family = AF_INET;
    host.sin_addr.S_un.S_addr = hostsockaddr.sin_addr.s_addr;
    
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
    std::thread keepAlive_t;
     while (true) {
        char choice;

        std::cout << "help : [t] (text message) [f] (file) [e] (shutdown) [l] (nastav velkost fragmentu)" << std::endl;
        std::cin >> choice;
        std::string msg, filename;
        int errPkt = 0;

        switch (choice) {
            case 't':
                ready.store(false);

                if (keepAlive_t.joinable()) {
                    keepAlive_t.join();
                }

                std::cout << "Message : ";
                std::cin >> msg;
                if(msg.size() > 0)
                    _resullt = sendMessage(msg, fragment, hostsockaddr, connectionSocket, TEXTM);

                keepAlive_t = std::thread(&keepAlive, hostsockaddr, connectionSocket);
                break;
            case 'f' :
                ready.store(false);

                if (keepAlive_t.joinable()) {
                    keepAlive_t.join();
                }

                filename = getFilename();
                
                if (!filename.empty())//osetri otvaranie suboru
                    _resullt = connect(filename, fragment, hostsockaddr, connectionSocket);

                if(_resullt > 0)
                    _resullt = sendFile(filename, fragment, hostsockaddr, connectionSocket, errPkt);
                if(_resullt > 0)
                    keepAlive_t = std::thread(&keepAlive, hostsockaddr, connectionSocket);

                break;
            case 'l':
                std::cin >> fragment;
                break;
            case 'e':
                ready.store(false);

                if (keepAlive_t.joinable()) {
                    keepAlive_t.join();
                }

                closesocket(connectionSocket);
                std::cout << "Client : client is shutting downn\nReturning to main screen..." << std::endl;
                return;

        }

        if (_resullt == SOCKET_ERROR) {
            std::cout << "Error : client is shutting down" << std::endl;
        }

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
