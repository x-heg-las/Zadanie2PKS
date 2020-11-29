#pragma once
#include <string>
#include <vector>
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

struct message {
	
	char* data;
	int offset;
	short len;
	short stream;
	message* next;
};

struct stream {
	message* data;
	short streamNumber;
	char finished;
};

class Message {

	public:
		Message(char* _data, short datalen, int offset) {

			data = new char[datalen];
			memcpy(data ,_data, datalen);
			len = datalen;
			int offset;
		}
		
		~Message() {
			delete[] data;
		}
		
		char* data;
		int offset;
		short len;
		


};


class Stream {

	public:
		Stream(short streamNumber, char _finished) {
			streamnumber = streamNumber;
			finished = _finished;
		}

		std::vector<Message> fragments;
		short streamnumber;
		char finished;

		void addFragment(Message msg) {
			fragments.push_back(msg);
		}
};

void freebuffer(stream data);
void analyzeHeader(header& protocol, char *buffer);
bool check(message stream);
void concat(stream* data, int id, char* buffer);
char* arq(header &protocol, int len);
void confirm();
int chooseService();
std::string getFilename();
std::string loadIP();
void fragmentMessage(fragment &message, int length, char* data, int fragmentLength);

unsigned short crc(char* data);
