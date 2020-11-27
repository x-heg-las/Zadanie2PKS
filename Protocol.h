#pragma once

#define HEADER_4 4
#define HEADER_8 8
#define HEADER_12 12

typedef unsigned char u_int8_t;

typedef struct {
	u_int8_t reserved : 1;
	u_int8_t reserved2 : 1;
	u_int8_t name : 1; // fragment obsahuje meno subor
	u_int8_t fragmented : 1;
	u_int8_t ack : 1;
	u_int8_t retry : 1;
	u_int8_t quit : 1;
	u_int8_t init : 1;

}FLAGS;



typedef struct {
	u_int8_t control : 1;
	u_int8_t text : 1;
	u_int8_t binary : 1;
	u_int8_t keep_alive : 1;
	u_int8_t reserved : 1;
	u_int8_t reserved2 : 1;
	u_int8_t len : 2;

}TYPE;

struct header {
	FLAGS flags;
	TYPE type;
	unsigned char data_len;
	unsigned char stream;
	int seq;
};

class Protocol
{
	public:
		FLAGS flags;
		TYPE type;
		unsigned short checksum;
		unsigned short dataLength;
		unsigned short streamNumber;
		int sequenceNumber;

};


