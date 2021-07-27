#pragma once
#include "Pkt_Def.h"

namespace error
{
	enum ErrorType { CRC, LENGTH, CMD, NONERROR };

	char * causeError(PktDef& pkt, ErrorType err)
	{
		pkt.CalcCRC();
		char* temp = pkt.GenPacket();
		switch (err)
		{
		case error::CRC:
			*(temp + (pkt.GetLength() - 1)) = ~*(temp + (pkt.GetLength() - 1));

			break;
		case error::LENGTH:
			*(temp + packetSize::HEADER - 1) = ~*(temp + (packetSize::HEADER - 1));
			break;
		case error::CMD:
			*(temp + packetSize::HEADER - 2) = ~*(temp + (packetSize::HEADER - 2));
			break;
		default:
			break;
		}

		return temp;
	}
}