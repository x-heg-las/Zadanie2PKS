#pragma once
#include <string>
#include "Protocol.h"
#define BAD_INPUT -1
#define SENDER 's'
#define RECIEVER 'r'



struct fragment {

	Protocol header;
	char* data;
	struct fragment* next;

};

struct recievedFragments {

};

char* arq(header protocol, Protocol data);
void confirm();
void analyzeHeader(Protocol& header, char* data);
int chooseService();
std::string getFilename();
std::string loadIP();
void fragmentMessage(fragment &message, int length, char* data, int fragmentLength);

unsigned short crc(char* data);
