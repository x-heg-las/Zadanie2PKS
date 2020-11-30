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
		Message(char* _data, short datalen, int _offset) {

			data = new char[datalen+1];
			memcpy(data ,_data, datalen);
			len = datalen;
			offset = _offset;
		}
		
		~Message() {
			delete [] data;
		}
		
		Message(const Message& mes) {
			data = new char[mes.len];
			memcpy(data, mes.data, mes.len);
			len = mes.len;
			offset = mes.offset;

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
			fragments.insert(fragments.begin() + msg.offset, msg);
			///fragments.push_back(msg);
		}
};

void freebuffer(stream data);
void analyzeHeader(header& protocol, char *buffer);
bool check(message stream);
void concat(std::vector<Stream>& streams, int id, char* buffer);
char* arq(header &protocol, int len);
void confirm();
int chooseService();
std::string getFilename();
std::string loadIP();
void fragmentMessage(fragment &message, int length, char* data, int fragmentLength, int type);

unsigned short crc(char* data);
