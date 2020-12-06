#pragma once
#include <string>
#include <vector>
#include "Protocol.h"
#include "include/checksum.h"
#include <fstream>
#include <unordered_map>

#define MAX_FRAG 1456

#define BAD_INPUT -1
#define SENDER 's'
#define RECIEVER 'r'
#define FILE 1
#define TEXTM 2
#define NAME 3


/// <summary>
///		Struktura predstavujuca jeden fragment posielanych dat
///		Drzi informacie o hlavicke a konkretne data
/// </summary>
struct fragment {

	Protocol header;
	char* data;

};

/// <summary>
///		Struktura predstavujuca jeden fragment posielanych dat
///		Drzi informacie o hlavicke a konkretne data
/// </summary>
struct message {
	
	char* data;
	int offset;
	short len;
	short stream;
	//message* next;
};


/// <summary>
///		Trieda spravujúca zaznamy tj. fragmenty
/// </summary>
class Message {

public:



	Message(char* _data, short datalen, int _offset) {

		data = new char[datalen + 1];
		memcpy(data, _data, datalen);
		len = datalen;
		offset = _offset;
	}

	~Message() {
		if(data)
			delete[] data;
	}

	Message(const Message& mes)
		: Message(mes.data, mes.len, mes.offset)
	{}

	Message& operator=(const Message& msg) {
		return *this = Message(msg);
	}

	Message(Message && msg) noexcept
	:data(nullptr), offset(BAD_INPUT), len(BAD_INPUT)
	{
		data = msg.data;
		len = msg.len;
		offset = msg.offset;


		msg.data = nullptr;
	}
	
	Message& operator=(Message&& msg) noexcept  {
		if (this != &msg) {
			delete[] data;
		}

		data = msg.data;
		len = msg.len;
		offset = msg.offset;


		msg.data = nullptr;

		return *this;
	}

		char* data;
		int offset;
		short len;
		


};

/// <summary>
///		Trieda spravujúca jednotlive toky dat jedneho suboru
/// </summary>
class Stream {

public:

	Stream() {
		streamnumber  = 0;
		finished = 0;
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
	std::vector<int> missing;
	std::unordered_map<int, int> ack;
	short streamnumber;
	char finished;

	void addFragment(Message& msg) {

		if (ack.find(msg.offset) == ack.end()) {
			if (fragments.size() > msg.offset) {
				fragments.insert(fragments.begin() + msg.offset, msg);
			}
			else
				fragments.insert(fragments.end(), msg);
		}
	}

	void initializeMissing(int lastSeq) {
		if (!finished) {
			missing = std::vector<int>(lastSeq + 1);
			for (int i = 0; i <= lastSeq; i++) {
				missing.at(i) = i;
			}
			finished = 1;
		}

	}


};

void freeData(std::vector<fragment>& data);
std::vector<char> requestPackets(Stream &stream);
void analyzeHeader(header& protocol, char *buffer);
void copyHeader(char* data, Protocol header);
int concat(std::vector<Stream>& streams, int id, char* buffer);
char* arq(header &protocol, int len);
int chooseService();
std::string getFilename();
char* saveFileTo(char* filename, char* filepath);
int loadPort();
std::string loadIP();
Stream *findStream(std::vector<Stream> &streams, short id);
int fragmentMessage(std::vector<fragment> & fragments,struct fragment message, int length, char* data, int fragmentLength, int type, unsigned short streamnum);
bool checkCompletition(std::vector<Stream> &stream, short streamid);
unsigned short crc(char* ptr, int length);
