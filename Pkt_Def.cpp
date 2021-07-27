#include "Pkt_Def.h"
#include <cstring>

PktDef::PktDef() {
	cmd.header.PktCount = 0;
	cmd.header.Sleep = 0;
	cmd.header.Status = 0;
	cmd.header.Drive = 0;
	cmd.header.Claw = 0;
	cmd.header.Arm = 0;
	cmd.header.ACK = 0;
	cmd.header.Padding = 0;
	cmd.header.length = packetSize::HEADER + packetSize::CRC;//edit by natan to set the length of the packet
	cmd.data = nullptr;
	cmd.CRC = 0;
	this->RawBuffer = nullptr;
}

PktDef::PktDef(char * c) {


	//memcpy(&cmd.header, &c[0], HEADERSIZE);
	memcpy(&cmd.header, c, 5); /* copies first 5 bytes: 4 bytes for PktCount, 1 byte for all of flags */
	cmd.header.length = *(c + 5); /* assigns the value of 6th byte to cmd.header.length */

	/*cmd.header.Sleep = c[4] & 1;
	cmd.header.Status = (c[4] >> 1) & 1;
	cmd.header.Drive = (c[4] >> 2) & 1;
	cmd.header.Claw = (c[4] >> 3) & 1;
	cmd.header.Arm = (c[4] >> 4) & 1;
	cmd.header.ACK = (c[4] >> 5) & 1;
	cmd.header.Padding = 0;
	memcpy(&cmd.header.length, &c[5], sizeof(cmd.header.length));*/

	SetBodyData(&c[6], getBodySize());

	memcpy(&cmd.CRC, &c[6 + (cmd.header.length - packetSize::HEADER - packetSize::CRC)], sizeof(cmd.CRC));

	if (this->getBodySize())//if the body is not empty
	{
		this->RawBuffer = new char[this->cmd.header.length];//Alocate and save the buffer
		memcpy(RawBuffer, c, cmd.header.length);
	}
}

PktDef::~PktDef()
{
	if (this->cmd.data != nullptr)
		delete[] this->cmd.data;
	if (this->RawBuffer != nullptr)
		delete[] this->RawBuffer;
}

void PktDef::SetCmd(CmdType type) {
	if (type != ACK)
	{
		*((char*)&this->cmd.header + sizeof(Header().PktCount)) = 0x00;
	}

	switch (type) {
	case SLEEP:
		cmd.header.Sleep = 1;
		break;
	case DRIVE:
		cmd.header.Drive = 1;
		break;
	case CLAW:
		cmd.header.Claw = 1;
		break;
	case ARM:
		cmd.header.Arm = 1;
		break;
	case ACK:
		cmd.header.ACK = 1;
		break;
	default:
		break;
	}
}

void PktDef::SetBodyData(char* c, int i) {
	cmd.data = new char[i];

	memcpy(cmd.data, c, i);
	this->cmd.header.length = packetSize::NoBody + i;
}

void PktDef::SetPktCount(int i) {
	cmd.header.PktCount = i;
}

/*
 * Take: void
 * Action: check CmdType flag configuration, and return identified type.
 * Return: CmdType.  Will throw if the configuration is invalid
 */
CmdType PktDef::GetCmd() const {

	CmdType rtn = NACK;


	unsigned char temp = (*(((unsigned char*)&cmd.header) + 4));//Corrected to not jump 4 sizes of the whole header (a total of 32 bytes!) Natan
	bool valid = false;
	int count = 0;
	for (int i = 0; i < 6 && count < 3; i++) /* check only 6 bits.  last 2 is pad */
		count += (temp >> i) & 1;

	switch (count) {
	case 0: /* all bitfields are 0 */
		rtn = NACK;
		valid = true;
		break;
	case 1: /* one field is 1 - command being sent */
		if (cmd.header.Sleep) {
			rtn = SLEEP;
			valid = true;
		}
		else if (cmd.header.Drive) {
			rtn = DRIVE;
			valid = true;
		}
		else if (cmd.header.Claw) {
			rtn = CLAW;
			valid = true;
		}
		else if (cmd.header.Arm) {
			rtn = ARM;
			valid = true;
		}
		else if (cmd.header.Status) {
			rtn = STATUS;
			valid = true;
		}
		break;
	}
	if (!valid) throw("ERROR: Invalid CmdType flag configuration found.");
	return rtn;
}

/*
 * Take: void
 * Action: get Ack status
 * Return: bool.  Will return false if exceptions thrown
 */
bool PktDef::GetAck() const {
	unsigned char temp = (*(((unsigned char*)&cmd.header) + 4));//Corrected to not jump 4 sizes of the whole header (a total of 32 bytes!) Natan
	int count = 0;
	for (int i = 0; i < 6 && count < 3; i++) /* check only 6 bits.  last 2 is pad */
		count += (temp >> i) & 1;
	return count == 2 && cmd.header.ACK && !cmd.header.Status ;
}

int PktDef::GetLength() const {
	return cmd.header.length;
}

char* PktDef::GetBodyData() const {
	return cmd.data;
}

int PktDef::GetPktCount()const {
	return cmd.header.PktCount;
}

//Checks if the CRC matches the current one on file
bool PktDef::CheckCRC(char * c, int currentSize) {



	unsigned char tempCRC = 0x00;

	for (size_t i = 0; i < currentSize - 1; i++)
	{
		char mask = 0x01;
		for (size_t j = 0; j < 8; j++)
		{
			char temp = *(c + i);
			temp = temp & mask;
			if (temp == mask)
				tempCRC++;
			mask = mask << 1;
		}
	}

	return tempCRC == c[currentSize - 1];
}

//Calculates are writes the CRC
void PktDef::CalcCRC() {

	char tempBuff[packetSize::MAXPACKETSIZE];

	packHeader(tempBuff);
	packBody(tempBuff + packetSize::HEADER);

	this->cmd.CRC = 0x00;//set to zero
	for (size_t i = 0; i < this->cmd.header.length - 1; i++)
	{
		char mask = 0x01;
		for (size_t j = 0; j < 8; j++)
		{
			char temp = *(tempBuff + i);
			temp = temp & mask;
			if (temp == mask)
				this->cmd.CRC++;
			mask = mask << 1;
		}
	}
}

void PktDef::packHeader(char * headerArray)
{
	int currentLoc = 0;
	std::memcpy(headerArray, &(this->cmd.header.PktCount), sizeof(this->cmd.header.PktCount));//move mem and add size of PktCount
	currentLoc += sizeof(this->cmd.header.PktCount);
	char * flags = ((char*)(&this->cmd.header.PktCount)) + currentLoc;
	headerArray[currentLoc++] = *flags;//copy over the flags to the current location  then add one
	std::memcpy(headerArray + currentLoc, &(this->cmd.header.length), sizeof(cmd.header.length));
}

void  PktDef::packBody(char * bodyArray)
{
	memcpy(bodyArray, this->cmd.data, this->getBodySize());
}

char* PktDef::GenPacket() {

	if (this->RawBuffer != nullptr)
		delete[] this->RawBuffer;

	this->RawBuffer = new char[this->cmd.header.length];

	packHeader(RawBuffer);//pack the header
	packBody(RawBuffer + packetSize::HEADER);
	this->CalcCRC();

	memcpy((RawBuffer + this->cmd.header.length) - packetSize::CRC, &this->cmd.CRC, packetSize::CRC);//create the CRC
	return this->RawBuffer;
}

int PktDef::getBodySize() const
{
	return this->cmd.header.length - packetSize::HEADER - packetSize::CRC;
}

void PktDef::resetPacket() {
	cmd.header.PktCount = 0;
	cmd.header.Sleep = 0;
	cmd.header.Status = 0;
	cmd.header.Drive = 0;
	cmd.header.Claw = 0;
	cmd.header.Arm = 0;
	cmd.header.ACK = 0;
	cmd.header.Padding = 0;
	cmd.header.length = packetSize::HEADER + packetSize::CRC;
	cmd.data = nullptr;
	cmd.CRC = 0;
	this->RawBuffer = nullptr;
}