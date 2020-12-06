#define _CRT_SECURE_NO_WARNINGS
#include "InitControll.h"
#include "Protocol.h"
#include "crc.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <fstream>
#include <string.h>
#include <sys/stat.h>
#include <locale>
#include <codecvt>
#include<ios>
#include<limits> 

/// <summary>
///  Funkcia pre spojenie fragmentov do jedneho bloku
/// </summary>
/// <returns>
///     Velkost dat
/// </returns>
int concat(std::vector<Stream> &streams, int id, char *buffer)
{
    int offset = 0;
    int cnt = 0;

   for (Stream& flow : streams) {
       if (flow.streamnumber == id) {

           for (Message& mes : flow.fragments) {

               memcpy((buffer)+offset, mes.data, mes.len);
               offset += mes.len;
           }
           streams.erase(streams.begin() + cnt);
           break;
       }
       cnt++;
    }

    return offset;
}

/// <summary>
///     Nastavi hodnoty hlavicky podla argumentov
/// </summary>
/// <returns>
///     Retazec predstavujuci hlavicku
/// </returns>
char* arq(header& protocol, int len)
{
    protocol.seq = len;
    protocol.type.len = protocol.type.len;
    protocol.data_len = 0;
    protocol.type.control = 1;

    if (protocol.flags.retry)
        protocol.flags.retry = 1;
    else
        protocol.flags.ack = 1;


    char* msg = new char[HEADER_12];

    memcpy(msg, &protocol, HEADER_12);

    return msg;
}

/// <summary>
///     Funkcia pre vyber modu
/// </summary>
int chooseService()
{
    char choice;
    std::cout << "Vyber mod, v ktorom chcete pracovat (reciever/sender)[r/s] ";
    std::cin >> choice;
    std::cin.clear();
    std::cin.ignore();

    if (choice == SENDER || choice == RECIEVER || choice == 'u')
        return choice;

    return BAD_INPUT;
}

/// <summary>
///     Zabezpecuje nacitanie cesty k pozadovanemu suboru
/// </summary>
/// <returns>
///     Cesta k suboru
/// </returns>
std::string getFilename()
{
    char filename[MAX_PATH];

    OPENFILENAME ofn;
    ZeroMemory(&filename, sizeof(filename));
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = (LPWSTR)"Text Files\0*.txt\0Any File\0*.*\0";
    ofn.lpstrFile = (LPWSTR)filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.lpstrTitle = (LPWSTR)"Select a File!";
    ofn.Flags = OFN_DONTADDTORECENT | OFN_FILEMUSTEXIST;

    if (GetOpenFileNameA((LPOPENFILENAMEA)&ofn))
    {
        std::cout << "You chose the file \"" << filename << "\"\n";
        std::string filepath = filename;
        replace(filepath.begin(), filepath.end(), '\\', '/');
        return filename;
    }
    else
    {
        switch (CommDlgExtendedError())
        {
        case CDERR_DIALOGFAILURE: std::cout << "CDERR_DIALOGFAILURE\n";   break;
        case CDERR_FINDRESFAILURE: std::cout << "CDERR_FINDRESFAILURE\n";  break;
        case CDERR_INITIALIZATION: std::cout << "CDERR_INITIALIZATION\n";  break;
        case CDERR_LOADRESFAILURE: std::cout << "CDERR_LOADRESFAILURE\n";  break;
        case CDERR_LOADSTRFAILURE: std::cout << "CDERR_LOADSTRFAILURE\n";  break;
        case CDERR_LOCKRESFAILURE: std::cout << "CDERR_LOCKRESFAILURE\n";  break;
        case CDERR_MEMALLOCFAILURE: std::cout << "CDERR_MEMALLOCFAILURE\n"; break;
        case CDERR_MEMLOCKFAILURE: std::cout << "CDERR_MEMLOCKFAILURE\n";  break;
        case CDERR_NOHINSTANCE: std::cout << "CDERR_NOHINSTANCE\n";     break;
        case CDERR_NOHOOK: std::cout << "CDERR_NOHOOK\n";          break;
        case CDERR_NOTEMPLATE: std::cout << "CDERR_NOTEMPLATE\n";      break;
        case CDERR_STRUCTSIZE: std::cout << "CDERR_STRUCTSIZE\n";      break;
        case FNERR_BUFFERTOOSMALL: std::cout << "FNERR_BUFFERTOOSMALL\n";  break;
        case FNERR_INVALIDFILENAME: std::cout << "FNERR_INVALIDFILENAME\n"; break;
        case FNERR_SUBCLASSFAILURE: std::cout << "FNERR_SUBCLASSFAILURE\n"; break;
        default: std::cout << "cancelled.\n";
        }
    }

    return "";
}

/// <summary>
///     Zabezpecuje ziskanie cesty k adresaru, do ktoreho ma byt ulozeny subor
/// </summary>
/// <returns>
///     Cesta k adresaru
/// </returns>
char* saveFileTo(char* filename, char* filepath)
{

    OPENFILENAMEA ofn;

    char szFileName[MAX_PATH] = "";
    char filters[MAX_PATH] = { 0 };
    char* post = strrchr(filename, '.');
    char* ptr = filters;
    sprintf(filters , "%s  ", post);
    sprintf(filters + strlen(post) +3, "*%s", post);

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;
    ofn.lpstrFilter = (LPCSTR) filters;
    ofn.lpstrFile = (LPSTR)filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = (LPCSTR)L"txt";
    

    if (GetSaveFileNameA((LPOPENFILENAMEA)&ofn)){
       
        std::string path = ofn.lpstrFile;
        strcpy(filepath, path.c_str());
    }
    else
    {
        switch (CommDlgExtendedError())
        {
        case CDERR_DIALOGFAILURE: std::cout << "CDERR_DIALOGFAILURE\n";   break;
        case CDERR_FINDRESFAILURE: std::cout << "CDERR_FINDRESFAILURE\n";  break;
        case CDERR_INITIALIZATION: std::cout << "CDERR_INITIALIZATION\n";  break;
        case CDERR_LOADRESFAILURE: std::cout << "CDERR_LOADRESFAILURE\n";  break;
        case CDERR_LOADSTRFAILURE: std::cout << "CDERR_LOADSTRFAILURE\n";  break;
        case CDERR_LOCKRESFAILURE: std::cout << "CDERR_LOCKRESFAILURE\n";  break;
        case CDERR_MEMALLOCFAILURE: std::cout << "CDERR_MEMALLOCFAILURE\n"; break;
        case CDERR_MEMLOCKFAILURE: std::cout << "CDERR_MEMLOCKFAILURE\n";  break;
        case CDERR_NOHINSTANCE: std::cout << "CDERR_NOHINSTANCE\n";     break;
        case CDERR_NOHOOK: std::cout << "CDERR_NOHOOK\n";          break;
        case CDERR_NOTEMPLATE: std::cout << "CDERR_NOTEMPLATE\n";      break;
        case CDERR_STRUCTSIZE: std::cout << "CDERR_STRUCTSIZE\n";      break;
        case FNERR_BUFFERTOOSMALL: std::cout << "FNERR_BUFFERTOOSMALL\n";  break;
        case FNERR_INVALIDFILENAME: std::cout << "FNERR_INVALIDFILENAME\n"; break;
        case FNERR_SUBCLASSFAILURE: std::cout << "FNERR_SUBCLASSFAILURE\n"; break;
        default: std::cout << "cancelled.\n";
        }
    }
  
    return nullptr;
}

/// <summary>
///     Uvolnenie alokovanych fragmentov
/// </summary>
void freeData(std::vector<fragment>& data)
{

    for (auto& mem : data) {
        delete[] mem.data;
    }
}

/// <summary>
///     Zisti sekvencne cisla neobdrzanych fragmentov
/// </summary>
std::vector<char>  requestPackets(Stream& stream)
{
    std::vector<char> missing;

    for (auto &missed : stream.missing) {
        if (missed != -1)
            missing.push_back(missed);
    }
    return missing;;
}

/// <summary>
///     Analyzuje hlavicku protokolu nad UDP
/// </summary>
void analyzeHeader(header& protocol, char* buffer) {

    ZeroMemory(&protocol, sizeof(protocol));

    protocol = *(header*)buffer;
    
    if (protocol.type.len == 2)
        protocol.seq = 0;
    if (protocol.type.len < 2) {
        protocol.data_len = 0;
    }

    return;
}

/// <summary>
///     Nacita IP adresu
/// </summary>
/// <returns>
///     Retazec s IP adresou
/// </returns>
std::string loadIP()
{

    std::string input;
    std::cout << "Zadaj IP adresu prijmacieho zariadenia s pouzitim notacie . (x.x.x.x)" << std::endl;

    std::cin >> input;

    return input;
}

/// <summary>
///     Nacita cislo portu
/// </summary>
/// <returns>
///     Cislo portu
/// </returns>
int loadPort() {

    int val = 0;
    std::cout << "Zadaj cislo portu [1024 - 65535]: ";

    std::cin >> val;
    std::cin.clear();
    std::cin.ignore();
    return val;
}
 
/// <summary>
///     Prevedie data hlavicky na retazec
/// </summary>
void copyHeader(char* data, Protocol header) {

    memcpy(data++,&header.flags,1);
    
    memcpy(data++,&header.type, 1);
   
    memcpy(data , &header.checksum, 2);
    
    if (header.type.len > 1) {
        
        data += 2;
        memcpy(data, &header.dataLength,  2);
      
        data += 2;
        memcpy(data, &header.streamNumber,  2);
        if(header.type.len > 2){
          
            data += 2;
            memcpy(data , &header.sequenceNumber,  4);
        }

    }

    return;
}

/// <summary>
///     Vytvori pole fragmetov podla nacitanych dat
/// </summary>
/// <returns>
///     Pocet fragmentov
/// </returns>
int fragmentMessage( std::vector<fragment> &vec, struct fragment message, int length, char* data, int fragmentLength, int type, unsigned short _stream) {

    fragment ptr;
    Protocol reference = message.header;
    int fragments = 0;

    ptr = message;
    
    for (int packed = 0; packed < length; packed += (fragmentLength - reference.type.len * 4), fragments++) {

        int size = 0;

        ptr.header = reference;

        if ((length - packed) > fragmentLength - (reference.type.len * 4))
            size = fragmentLength - (reference.type.len * 4);
        else {
            size = length - packed ;
            ptr.header.type.control = 1;
            ptr.header.flags.quit = 1;
        }

        ptr.data = new char[fragmentLength];

        if (!fragments) {
            ptr.header.flags.init = 1;
            ptr.header.type.control = 1;
        }
        
        ptr.header.checksum = 0;
        ptr.header.dataLength = size;
        ptr.header.sequenceNumber = fragments;

        if (type == NAME) {
            ptr.header.flags.name = 1;
            ptr.header.type.binary = 1;
        }
       
        ptr.header.streamNumber = _stream;
        copyHeader(ptr.data, ptr.header);
        std::copy_n(data, size, ptr.data + (reference.type.len * 4));

        ptr.header.checksum = crc(ptr.data, size + (reference.type.len * 4));
        copyHeader(ptr.data, ptr.header);

        
        data += size;
        
        vec.push_back(ptr);
    }

    return fragments;

}

/// <summary>
///     Zisti ci boli prijate vsetky fragmenty
/// </summary>
bool checkCompletition(std::vector<Stream>& stream,  short streamid)
{
    Stream* data = findStream(stream, streamid);

    int counter = 0;
    for (auto &member : data->fragments) {
        data->missing.at(counter) = -1;

        if (member.offset != counter)
            return false; 
        counter++;
    }

    return true;
}

/// <summary>
///     Najde pozadovany tok dat podla cisla toku v hlavicke
/// </summary>
Stream *findStream(std::vector<Stream>& streams, short id)
{
    Stream* str;
    
    std::find_if(streams.begin(),
        streams.end(),
        [&var = id, &se = str]
    (Stream& s) -> bool { 
            if (s.streamnumber == var) { 
                se = &s; return true;
            }
            se = nullptr;  
            return false;       
        
        });


    return str;
}

/// <summary>
///     Vypocita hodnotu CRC dat
/// </summary>

unsigned short crc(char* ptr, int length) {

    uint16_t crc_ccitt_1d0f_val = 0x1d0f;

    for (int i = 0; i < length; i++) {

        if (i > 1 && i < 4) { // preskocim polia v havicke s CRC 
            crc_ccitt_1d0f_val = update_crc_ccitt(crc_ccitt_1d0f_val, '\0');
        }else
            crc_ccitt_1d0f_val = update_crc_ccitt(crc_ccitt_1d0f_val, (unsigned char)*ptr);

        ptr++;
    }

    return crc_ccitt_1d0f_val;
}
