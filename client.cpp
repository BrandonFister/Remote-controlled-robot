/*#include <Windows.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "MySocket.h"
#include "Pkt_Def.h"
#include "CmdObj.h"
#include "BuffObj.h"
#include "Log.h"
#include "Gamepad.h"


//To Do:  receive ack packet, parse command string using iterators

bool ExeComplete;


/*
* Command Thread
*/
void CommandThread(MySocket roboSocket, std::string logfile, bool& complete, CmdObj &co) {
	//MySocket roboSocket(SocketType::CLIENT, ip, port, ConnectionType::TCP, 0);
	roboSocket.ConnectTCP();

	Log log(logfile);
	std::string cmdString;									//command received from gui
//	char* txBuffer = "\0";									//char* to be sent via the socket
	char* txBuffer = nullptr;
	//	char* rxBuffer = "\0";
	char rxBuffer[255] = { 0 };
	std::chrono::milliseconds ms(500);						//half a second for the iteration pauses
	unsigned int count = 1;									//to start the packet count

	PktDef transferPacket;

	do {
		//		transferPacket = nullptr; /* this variable is not a pointer, so this should not be possible  c-t*/

		if (!co.isEmpty()) {
			cmdString = co.getCmd();
			std::string command = "UNKNOWN", extra1 = "UNKNOWN";
			//			unsigned char dur = '\0'; /* this is same as just 0 c-t*/
			unsigned char dur = 0;
			size_t pos;										//position in command string

			pos = cmdString.find(",");						//Retreives the GUI's command and parses it into smaller bits to fill the packet header
			if (pos != std::string::npos) {
				command = cmdString.substr(0, pos);
				cmdString.replace(0, pos + 1, "");

				pos = cmdString.find(",");
				if (pos != std::string::npos) {
					extra1 = cmdString.substr(0, pos);
					cmdString.replace(0, pos + 1, "");

					if (command == "DRIVE" && cmdString.size() > 0)
						dur = cmdString.at(0);
				}
			}

			transferPacket.SetPktCount(count);				//Sets Packet count

			if (command == "DRIVE") {						//Drive command
				MotorBody mtr;
				if (extra1 == "FORWARD") {
					mtr.Direction = Comd::FORWARD;
				}
				else if (extra1 == "BACKWARDS") {
					mtr.Direction = Comd::BACKWARDS;
				}
				else if (extra1 == "RIGHT") {
					mtr.Direction = Comd::RIGHT;
				}
				else if (extra1 == "LEFT") {
					mtr.Direction = Comd::LEFT;
				}
				mtr.Duration = dur;
				transferPacket.SetCmd(CmdType::DRIVE);
				transferPacket.SetBodyData((char*)&mtr, sizeof(MotorBody));
			}
			else if (command == "ARM") {					//Arm command
				ActuatorBody atr;
				if (extra1 == "UP")
					atr.Action = Comd::UP;
				else if (extra1 == "DOWN")
					atr.Action = Comd::DOWN;
				transferPacket.SetCmd(CmdType::ARM);
				transferPacket.SetBodyData((char*)&atr, sizeof(ActuatorBody));
			}
			else if (command == "CLAW") {					//Claw Command
				ActuatorBody atr;
				if (extra1 == "OPEN")
					atr.Action = Comd::OPEN;
				else if (extra1 == "CLOSE")
					atr.Action = Comd::CLOSE;
				transferPacket.SetCmd(CmdType::CLAW);
				transferPacket.SetBodyData((char*)&atr, sizeof(ActuatorBody));
			}
			else if (command == "SLEEP") {
				transferPacket.SetCmd(CmdType::SLEEP);		//Sleep Command
			}

			count++;


			//Write sent to log file

			std::string s;

			if (dur)
				s = "Command sent " + std::to_string(transferPacket.GetCmd()) + " " + extra1 + " " + std::to_string(dur) + " seconds";
			else
				s = "Command sent " + std::to_string(transferPacket.GetCmd()) + " " + extra1;

			log(s);

			txBuffer = transferPacket.GenPacket();
			//			roboSocket.SendData(txBuffer, sizeof(transferPacket));
			roboSocket.SendData(txBuffer, transferPacket.GetLength());

			roboSocket.GetData(rxBuffer);

			PktDef receivePacket(rxBuffer);

			count++;

			//Write received packet to log file
			if (receivePacket.GetAck())  /* if ACK */
				s = "Packet Acknowledged, executing command.";
			else if (receivePacket.GetCmd() == NACK) { /* if not ACK but NACK */
				s = "Packet Negative Acknowledgement: ";
				s += receivePacket.GetBodyData();
			}
			else { /* if neither */
				s = "Invalid Packet Received.";
				//				s += receivePacket.GetBodyData();
			}

			log(s);

			std::this_thread::sleep_for(ms);

			transferPacket.resetPacket();
			receivePacket.resetPacket();
			delete[] txBuffer;
			std::memset(rxBuffer, 0, 255);
		}
	} while (transferPacket.GetCmd() != CmdType::SLEEP);

	roboSocket.DisconnectTCP();
}

/*
 * Telemetry Thread
 * TelThread is the main telemetry thread that spawns recvThread and logs to files. recvThread only recieves
 */

void recvThread(MySocket sock, BuffObj& rcvBuff, bool& complete)
{
	sock.ConnectTCP();
	char buff[DEFAULT_SIZE];
	int size = 0;
	while (!complete) {
		size = sock.GetData(buff);
		rcvBuff.add(buff, size);
		/* no need to pause as GetData() will wait for new input */
	}
	sock.DisconnectTCP();
}

void TelThread(MySocket sock, std::string logname, bool& complete)
{
	struct StatusBody {
		unsigned short int sonar = 0;
		unsigned short int ArmPos = 0;
		unsigned char drive : 1;
		unsigned char arm_up : 1;
		unsigned char arm_down : 1;
		unsigned char claw_open : 1;
		unsigned char claw_closed : 1;
		unsigned char padding : 1;
	};

	BuffObj buff;
	Log log(logname);
	enum { STATUS_PACKET_SIZE = 12 };
	std::chrono::milliseconds ms(500);
	std::thread recver = std::thread(recvThread, std::move(sock), std::ref(buff), std::ref(complete));
	while (!complete) {
		if (!buff.isEmpty()) {
			//			std::unique_ptr<char> temp = buff.get();
			BuffInfo tempBuff = buff.get();
			bool success = false;
			if (PktDef().CheckCRC(tempBuff.buff, tempBuff.size) && tempBuff.size == STATUS_PACKET_SIZE) {
				PktDef p(tempBuff.buff);
				if (p.GetCmd() == STATUS && p.GetLength() == tempBuff.size) { /* STATUS packet, and length is the same as received length */
					success = true;
					/* display RAW */
					char* raw = new char[tempBuff.size + 1];
					std::memcpy(raw, tempBuff.buff, tempBuff.size);
					raw[tempBuff.size] = '\0'; /* to print it on the log */
					log(raw);
					/* display intepreted */
					StatusBody s;
					std::memset((char*)&s, 0, sizeof(StatusBody));
					std::memcpy((char*)&s, p.GetBodyData(), STATUS_PACKET_SIZE);
					std::string output = "";
					output += "Sonar: " + std::to_string(s.sonar) + "Arm Position: " + std::to_string(s.ArmPos);
					if (s.drive)
						output += ", DRIVING";
					else
						output += ", STOPPED";

					if (s.arm_down && !s.arm_up)
						output += ", Arm is Down";
					else if (s.arm_up && !s.arm_down)
						output += ", Arm is Up";
					else
						output += ", Arm must be Schrodigner's Cat";

					if (s.claw_closed && !s.claw_open)
						output += ", Claw is Closed";
					else if (s.claw_open && !s.claw_closed)
						output += ", Claw is Open";
					else
						output += ", Claw achieved quantum superposition";

					log(output);
				}
			}
			if (!success) {
				std::string err = "ERROR: Invalid Packet";
				log(err);
				std::cout << err << std::endl;
			}
		}
		std::this_thread::sleep_for(ms);
	}
	recver.join();
}

/*
 * main

int main(int argc, char* argv[]) {
	CoInitialize(0);
	MySocket cmdSocket(CLIENT, "127.0.0.1", 27000, TCP, 128);
	MySocket telSocket(CLIENT, "127.0.0.1", 27501, TCP, 128);
	CmdObj co;
	std::string logFile = "log.txt";
	bool ExeComplete = false;

	// TODO: Create Threads
	std::thread cmdThd(CommandThread, std::move(cmdSocket), logFile, std::ref(ExeComplete), std::ref(co));
	std::thread telThd(TelThread, std::move(telSocket), logFile, std::ref(ExeComplete));

	Gamepad gamepad;

	bool wasConnected = true;

	while (true)
	{
		Sleep(100);

		if (!gamepad.Refresh())
		{
			if (wasConnected)
			{
				wasConnected = false;

				std::cout << "Please connect an Xbox controller." << std::endl;
			}
		}
		else
		{
			if (!wasConnected)
			{
				wasConnected = true;

				std::cout << "Controller connected on port " << gamepad.GetPort() << std::endl;
			}

			std::string ACTION;
			std::string DIRECTION;
			std::string TIME = "1";
			std::string COMMAND = "A";

			gamepad.Refresh();
			if (gamepad.IsPressed(0x1000)) { // A: DRIVE
				std::cout << "(A) button pressed" << std::endl;
				ACTION = "DRIVE";
				while (true) {
					gamepad.Refresh();
					if (gamepad.IsPressed(0x0001)) { //FORWARD
						std::cout << "UP button pressed" << std::endl;
						DIRECTION = "FORWARD";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
					else if (gamepad.IsPressed(0x0002)) { //BACKWARD
						std::cout << "DOWN button pressed" << std::endl;
						DIRECTION = "BACKWARD";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
					else if (gamepad.IsPressed(0x0004)) { //LEFT
						std::cout << "LEFT button pressed" << std::endl;
						DIRECTION = "LEFT";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
					else if (gamepad.IsPressed(0x0008)) { //RIGHT
						std::cout << "RIGHT button pressed" << std::endl;
						DIRECTION = "RIGHT";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
				}
			}
			else if (gamepad.IsPressed(0x2000)) { // B: ARM
				std::cout << "(b) button pressed" << std::endl;
				ACTION = "ARM";
				while (true) {
					gamepad.Refresh();
					if (gamepad.IsPressed(0x0001)) { // UP
						std::cout << "UP button pressed" << std::endl;
						DIRECTION = "UP";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
					else if (gamepad.IsPressed(0x0002)) { // DOWN
						std::cout << "DOWN button pressed" << std::endl;
						DIRECTION = "DOWN";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
				}
			}
			else if (gamepad.IsPressed(0x4000)) { // X: CLAW
				std::cout << "(x) button pressed" << std::endl;
				ACTION = "CLAW";
				while (true) {
					gamepad.Refresh();
					if (gamepad.IsPressed(0x0080)) { // R: OPEN
						std::cout << "R button pressed" << std::endl;
						DIRECTION = "OPEN";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
					else if (gamepad.IsPressed(0x0040)) { // L: CLOSE
						std::cout << "L button pressed" << std::endl;
						DIRECTION = "CLOSE";
						COMMAND = ACTION + "," + DIRECTION + "," + TIME;
						co.addCmd(COMMAND);
						break;
					}
				}
			}
			else if (gamepad.IsPressed(0x8000)) { // Y: SLEEP
				std::cout << "(y) button pressed" << std::endl;
				ACTION = "SLEEP";

				// TODO: Join threads
				telThd.join();
				cmdThd.join();

				ExeComplete = true;
				break;
			}

		}
	}
	CoUninitialize();
	return 0;
}
*/
