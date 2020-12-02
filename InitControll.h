#pragma once
#include <string>
#include <vector>
#include "Protocol.h"
#include <fstream>

#define BAD_INPUT -1
#define SENDER 's'
#define RECIEVER 'r'
#define FILE 1
#define TEXTM 2
#define NAME 3



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

	Stream() {
		streamnumber  = 0;
		finished = 1;

	}

	Stream(const Stream& str) {
		streamnumber = str.streamnumber;
		finished = 0;
		fragments = str.fragments;
	}

	Stream(short streamNumber, char _finished) {
		streamnumber = streamNumber;
		finished = _finished;
	}

	std::vector<Message> fragments;
	std::vector<char> missing; 
	short streamnumber;
	char finished;

	void addFragment(Message msg) {
		if (fragments.size() > msg.offset)
			fragments.insert(fragments.begin() + msg.offset, msg);
		else
			fragments.insert(fragments.end(), msg);
		
	}

	void initializeMissing(int lastSeq) {
		missing = std::vector<char>(lastSeq + 1);
		for (int i = 0; i <= lastSeq; i++) {
			missing.at(i) = i;
		}

	}
};

std::vector<char> requestPackets(Stream &stream);
void analyzeHeader(header& protocol, char *buffer);
bool check(message stream);
int concat(std::vector<Stream>& streams, int id, char* buffer);
char* arq(header &protocol, int len);
int chooseService();
std::string getFilename();
std::string loadIP();
Stream *findStream(std::vector<Stream> &streams, short id);
void fragmentMessage(fragment &message, int length, char* data, int fragmentLength, int type);
bool checkCompletition(std::vector<Stream> &stream, short streamid);
unsigned short crc(char* data);
