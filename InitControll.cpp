#include "InitControll.h"
#include "Protocol.h"
#include <iostream>
#include <vector>
#include <algorithm>
#include <windows.h>
#include <fstream>

int concat(std::vector<Stream> &streams, int id, char *buffer)
{
    int offset = 0;
   /*
   std::find_if(streams.begin(),
        streams.end(),
        [&var = id, &stream = s, &con = confirmed]
   (Stream& s) -> bool { if (s.streamnumber == var) { s = *stream; con = true;  return true;
   }return false;  });
   */
    int cnt = 0;
   for (Stream& flow : streams) {
       if (flow.streamnumber == id) {
           /*
           std::sort(
               flow.fragments.begin(), flow.fragments.end(),
               [] (const Message &a,const Message &b) -> bool {
                   return a.offset < b.offset;
               }
           );
           */

         

           for (Message& mes : flow.fragments) {

               memcpy((buffer)+offset, mes.data, mes.len);
               offset += mes.len;
           }
           streams.erase(streams.begin() + cnt);
           break;
       }
       cnt++;
    }

    //int offset = 0;

    /*
    if (!check(*ptr)) {
        
        std::cout << "Server : Chyba v InitControl;" << std::endl;
        return;
    }
    

    int iter = 0;

    for (Message& mes : s->fragments) {
        if (mes.offset != iter)
            return;
        iter++;
    }
        

    for (Message& mes : s->fragments) {

        memcpy((buffer) + offset, mes.data, mes.len);
        offset += mes.len;
    }
    */

    return offset;
}

char* arq(header &protocol, int len)
{
    if (protocol.flags.retry) {
        protocol.data_len = 0;
        protocol.flags.retry = 1;
        protocol.seq = len;
        protocol.type.len = 2;
        protocol.type.control = 1;
    }
    else {
        protocol.data_len = 0;
        protocol.flags.ack = 1;
        protocol.seq = len;
        protocol.type.len = 2;
        protocol.type.control = 1;
    }
    
    char *msg = new char[HEADER_12];
    
    memcpy(msg, &protocol, HEADER_12);

    return msg;
}


int chooseService()
{
    char choice;
    std::cout << "Vyber mod, v ktorom chcete pracovat (reciever/sender)[r/s] ";
    std::cin >> choice;
    
    if (choice == SENDER || choice == RECIEVER)
        return choice;

    return BAD_INPUT;
}

std::string getFilename()
{
       
        char filename[MAX_PATH];

        OPENFILENAME ofn;
        ZeroMemory(&filename, sizeof(filename));
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;  // If you have a window to center over, put its HANDLE here
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
            // All this stuff below is to tell you exactly how you messed up above. 
            // Once you've got that fixed, you can often (not always!) reduce it to a 'user cancelled' assumption.
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
    
    return "" ;
}

std::vector<char>  requestPackets(Stream& stream)
{
    std::vector<char> missing;

    for (auto missed : stream.missing) {
        if (missed != -1)
            missing.push_back(missed);
    }
    return missing;;
}

/*
void freebuffer(stream *data)
{
    stream* ptr = data;
    while (ptr) {
        stream* prew = ptr;
        ptr = ptr->next;
        delete prew;
    }
    delete data;

}
*/
void analyzeHeader(header& protocol, char* buffer) {

    ZeroMemory(&protocol, sizeof(protocol));

    protocol = *(header*)buffer;
    
    if (protocol.type.len == 2)
        protocol.seq = 0;
    if (protocol.type.len < 2) {
        protocol.data_len = 0;
        protocol.stream = 0;
    }

    return;
}

bool check(message stream)
{
    message* ptr = &stream;

    int offset = 0;
    while (ptr) {
        if (ptr->offset != offset)
            return false;
        ptr = ptr->next;
        offset++;
    }
    return true;
}


std::string loadIP()
{

    std::string input;
    std::cout << "Zadaj IP adresu prijmacieho zariadenia s pouzitim notacie . (x.x.x.x)" << std::endl;

    std::cin >> input;

    return input;
}

void copyHeader(char* data, Protocol header) {

    //std::copy_n(&header.flags,1,data);
    memcpy(data++,&header.flags,1);
    //std::copy_n(&header.type, 1, data + 1);
    memcpy(data++,&header.type, 1);
    //std::copy_n(&header.checksum, 2, data + 1);
    memcpy(data , &header.checksum, 2);
    
    if (header.type.len >= 1) {
        //std::copy_n(&header.dataLength, 2, data + 2);
        data += 2;
        memcpy(data, &header.dataLength,  2);
        //std::copy_n(&header.streamNumber, 2, data + 2);
        data += 2;
        memcpy(data, &header.streamNumber,  2);
        if(header.type.len >= 2){
            //std::copy_n(&header.sequenceNumber, 4, data + 2);
            data += 2;
            memcpy(data , &header.sequenceNumber,  4);
        }

    }

    return;
}

void fragmentMessage( fragment &message, int length, char* data, int fragmentLength, int type) {

    fragment* ptr = NULL;
    Protocol reference = message.header;

    ptr = &message;
    
    for (int packed = 0, fragments = 0; packed <= length; packed += (fragmentLength - reference.type.len * 4), fragments++) {

        ZeroMemory(&ptr->header, sizeof(header));
        int size = 0;

        ptr->header = reference;

        if ((length - packed) >= fragmentLength - (reference.type.len * 4))
            size = fragmentLength - (reference.type.len * 4);
        else {
            size = length - packed ;
            ptr->header.type.control = 1;
            ptr->header.flags.quit = 1;
        }

        ptr->data = new char[fragmentLength];

        if (!fragments) {
            ptr->header.flags.init = 1;
            ptr->header.type.control = 1;
        }
        
        ptr->header.checksum = crc(data);
        ptr->header.dataLength = size;
        ptr->header.sequenceNumber = fragments;
        ptr->next = new fragment;

        if (type == NAME) {
            ptr->header.flags.name = 1;
            ptr->header.type.binary = 1;
        }
       

        copyHeader(ptr->data, ptr->header);

        std::copy_n(data, size, ptr->data + (reference.type.len * 4));
        data += size;
 
        ptr = ptr->next;
        

    }
    ZeroMemory(ptr, sizeof(struct fragment));

}

bool checkCompletition(std::vector<Stream>& stream,  short streamid)
{
    Stream* data = findStream(stream, streamid);

    int counter = 0;
    for (auto member : data->fragments) {
        data->missing.at(counter) = -1;

        if (member.offset != counter)
            return true; //daj false potom
        counter++;
    }

  
    return true;
}

Stream *findStream(std::vector<Stream>& streams, short id)
{
    Stream* str;
   std::find_if(streams.begin(),
        streams.end(),
        [&var = id, &se = str]
    (Stream& s) -> bool { if (s.streamnumber == var) { se = &s; return true; }return false;  });


    return str;
}



unsigned short crc(char* data){



    return 0;
}

