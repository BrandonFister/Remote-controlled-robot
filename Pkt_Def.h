#pragma once
#include <cstring>
#include "Log.h"


struct Header {
	unsigned int PktCount;
	unsigned char Sleep : 1;
	unsigned char Status : 1;
	unsigned char Drive : 1;
	unsigned char Claw : 1;
	unsigned char Arm : 1;
	unsigned char ACK : 1;
	unsigned char Padding : 2;
	unsigned char length;
};

struct MotorBody {
	unsigned char Direction;
	unsigned char Duration;
};

struct ActuatorBody {
	unsigned char Action;
};

enum Comd { FORWARD=1, BACKWARDS, RIGHT, LEFT, UP, DOWN, OPEN, CLOSE };
enum CmdType { DRIVE=1, SLEEP, ARM, CLAW, ACK, NACK, STATUS };
enum packetSize { HEADER = 6, CRC = 1, MAXPACKETSIZE = 263, NoBody=7 };//In Bytes

struct CmdPacket {
	Header header;
	char* data;
	char CRC;
};

class PktDef {
private:
	CmdPacket cmd;
	char *RawBuffer;
public:
	PktDef();
	PktDef(char*);
	~PktDef();
	void SetCmd(CmdType);
	void SetBodyData(char*, int);
	void SetPktCount(int);
	CmdType GetCmd() const;
	bool GetAck() const;
	int GetLength() const;
	char *GetBodyData() const;
	int GetPktCount() const;
	//sets the length of the current object as well
	bool CheckCRC(char*, int length);
	//sets the CRC based off the current values
	void CalcCRC();
	void packHeader(char * headerArray);
	void packBody(char * bodyArray);
	// Generates the CRC as well
	char *GenPacket();
	int getBodySize()const;
	void resetPacket();
};