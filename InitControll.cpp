#include "InitControll.h"
#include <iostream>
#include <algorithm>
#include <windows.h>


int chooseService()
{
	char choice;
	std::cout << "Vyber mod, v ktorom chcete pracovat (reciever\sender)\n[r/s] ";
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
    
	return "Failure !!!!" ;
}

std::string loadIP()
{

	std::string input;
	std::cout << "Zadaj IP adresu prijmacieho zariadenia s pouzitim \".\" notacie . (x.x.x.x)" << std::endl;

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
    
    if (header.type.len > 1) {
        //std::copy_n(&header.dataLength, 2, data + 2);
        data += 2;
        memcpy(data, &header.dataLength,  2);
        //std::copy_n(&header.streamNumber, 2, data + 2);
        data += 2;
        memcpy(data, &header.streamNumber,  2);
        if(header.type.len > 2){
            //std::copy_n(&header.sequenceNumber, 4, data + 2);
            data += 2;
            memcpy(data , &header.sequenceNumber,  4);
        }

    }

    return;
}

struct fragment* fragmentMessage(struct fragment *message, int length, char* data, int fragmentLength) {

    struct fragment* ptr = NULL;
    
    ptr = new struct fragment;
    message = ptr;
    for (int packed = 0, fragment = 0; packed <= length; packed += fragmentLength, fragment++) {

        ZeroMemory(&ptr->header, sizeof(header));
        int size = 0;

        if ((length - packed) >= fragmentLength)
            size = fragmentLength - HEADER_12;
        else {
            size = length - packed - HEADER_12;
            ptr->header.type.control = 1;
            ptr->header.flags.quit = 1;
        }

        ptr->data = new char[size];

        if (!fragment) {
            ptr->header.flags.init = 1;
            ptr->header.type.control = 1;
        }

        ptr->header.type.len = 3;
        ptr->header.flags.fragmented = 1;
        ptr->header.type.text = 1;
        ptr->header.checksum = crc(data);
        ptr->header.dataLength = size;
        ptr->header.sequenceNumber = fragment;
        ptr->next = new struct fragment;

        copyHeader(ptr->data, ptr->header);

        std::copy_n(data, size, ptr->data + HEADER_12);
        data += size;
        ptr = ptr->next;


    }
    ptr = NULL;

    return message;
}

unsigned short crc(char* data){



    return 0;
}

