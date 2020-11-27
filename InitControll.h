#pragma once
#include <string>
#include "Protocol.h"
#define BAD_INPUT -1
#define SENDER 's'
#define RECIEVER 'r'

int chooseService();
std::string getFilename();
std::string loadIP();
struct fragment* fragmentMessage(struct fragment* message, int length, char* data, int fragmentLength);

unsigned short crc(char* data);

struct fragment {

	Protocol header;
	char* data;
	struct fragment* next;

};