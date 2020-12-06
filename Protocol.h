#pragma once

#define HEADER_4 4
#define HEADER_8 8
#define HEADER_12 12

typedef unsigned char u_int8_t;

/// <summary>
///		Struktura predstavujuca pole FLAGS v hlavicke protokolu
/// </summary>
typedef struct {
	u_int8_t resesend : 1;
	u_int8_t reserved : 1;
	u_int8_t name : 1; // fragment obsahuje meno subor
	u_int8_t fragmented : 1;
	u_int8_t ack : 1;
	u_int8_t retry : 1;
	u_int8_t quit : 1;
	u_int8_t init : 1;

}FLAGS;


/// <summary>
///		Struktura predstavujuca pole TYPE v hlavicke protokolu
/// </summary>
typedef struct {
	u_int8_t control : 1;
	u_int8_t text : 1;
	u_int8_t binary : 1;
	u_int8_t keep_alive : 1;
	u_int8_t reserved : 1;
	u_int8_t reserved2 : 1;
	u_int8_t len : 2;

}TYPE;

/// <summary>
///  Štruktura predstavujuca protokol nad UDP
/// </summary>
struct header {
	FLAGS flags;
	TYPE type;
	unsigned short chcecksum;
	unsigned short data_len;
	unsigned short stream;
	int seq;
};

/// <summary>
///		Trieda predstavujuca protokol nad UDP
/// </summary>
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


