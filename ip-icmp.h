// ------------------------------------------------
// ICMP-IP Header File.
//
// Author: Jesus Camara (jesus.camara@infor.uva.es)
// ------------------------------------------------

// Network Libraries
#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/ip.h>
#include<arpa/inet.h>
#include<netdb.h>

/* IPv4 Header Definition
 *
   |  4 bits  |  4 bits  |       8 bits        |  3 bits  |           13 bits             |
   | ======== | ======== | =================== | ======================================== |
   |  Version |    HL	 |   Type of Service   |               Total Length               |
   | ------------------------------------------------------------------------------------ |
   |                Identifier                 |   Flags  |        Fragment Offset        |
   | ------------------------------------------------------------------------------------ |
   |     Time-to-Live    |       Protocol      |              Header Checksum             |
   | ------------------------------------------------------------------------------------ |
   |                                     Source Address                                   |
   | ------------------------------------------------------------------------------------ |
   |                                  Destination Address                                 |
   | ------------------------------------------------------------------------------------ |
 */
typedef struct sIPHeader {
		unsigned char VHL;		// IP_Version + Header_Length
		unsigned char ToS;		// Type of Service
		short int TotLeng;		// Total Datagram Length.
		short int ID;			// Datagram ID.
		short int FlagOff;		// Flag + Fragment_Offset
		unsigned char TTL;		// Time-to-Live
		unsigned char Protocol;		// Higher-Level Protocol.
		unsigned short int Checksum;	// Header Checksum Value.
		struct in_addr iaSrc;		// Source IP Address
		struct in_addr iaDst;		// Destination IP Address
} IPHeader;

/* ICMP Header Definition
 *
	|     1 byte    |     1 byte    |     	      2 bytes     	|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|      Type     |      Code     |            Checksum           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct sICMPHeader {
		unsigned char  type;		// Type of ICMP Message
		unsigned char  code;		// Code Related to Type
		unsigned short int checksum;	// Checksum of the ICMP Message
} ICMPHeader;

/*	ICMP Timestamp Message
 *
 *	Originate Timestamp: the time the sender last touched the message before sending it.
 *	Receive   Timestamp: the time the echoer first touched it on receipt.
 *	Transmit  Timestamp: the time the echoer last touched the message before sending it.
 *
	|     1 byte    |     1 byte    |     	      2 bytes     	|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|      Type     |      Code     |            Checksum           |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|           Identifier          |         Sequence Number       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                      Originate Timestamp                      |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                      Receive Timestamp                        |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|                      Transmit Timestamp                       |
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct sTimeStamp {
		ICMPHeader icmpHdr;		// ICMP Header
		unsigned short int pid;		// Process ID.
		unsigned short int sequence;	// Sequence Number.
		int originate;			// Source Timestamp
		int receive;			// Destination Timestamp.
		int transmit;			// Transmission Timestamp
} TimeStamp;

/* ICMP Timestamp Reply Message
 *
	|		   20 bytes	        |		20 bytes		| 
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
	|      		   IP Header	        |             ICMP Message		|
	+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
typedef struct sTimeStampReply {
		IPHeader  ipHdr;	// IP Header
		TimeStamp icmpMsg;	// ICMP Message
} TimeStampReply;

