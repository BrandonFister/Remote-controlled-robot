#pragma once
#include <windows.networking.sockets.h>
#pragma comment(lib, "Ws2_32.lib")
#include <iostream>
#include <fstream>
#include <string>
#include "Pkt_Def.h"

enum SocketType { CLIENT, SERVER };
enum ConnectionType { TCP, UDP };
const int DEFAULT_SIZE = packetSize::MAXPACKETSIZE;

class MySocket
{
	//Dynamically allocated RAW buffer space for communication activities
	char * m_Buffer;
	// configured as a TCP / IP Server
	SOCKET m_WelcomeSocket;
	//used for client/server communications (both TCP and UDP)
	SOCKET m_ConnectionSocket;
	//Stores connection information
	struct sockaddr_in m_SvrAddr;
	//Stores the reply address for a UDP connection
	struct sockaddr_in m_RespAddr;
	//holds the type of socket the MySocket object is initialized to
	SocketType m_mySocket;
	//Holds the IPv4 IP Address string
	std::string m_IPAddr;
	//Holds the port number to be used
	int m_Port;
	//defines the Transport Layer protocol being used(TCP / UDP)
	ConnectionType m_connectionType;
	//flag to determine if a socket has been initialized or not
	bool m_bConnect;
	//Store the maximum number of bytes the buffer is allocated to.
	int m_MaxSize;

	//WinsockData
	WSADATA wsa_data;

	//--------------FUNCTIONS------------------------
	//a private member function that initializes the Winsock DLL. This will return true/false based on successful Winsock initialization
	bool StartWSA();
	//used to create TCP Sockets
	SOCKET initialize_tcp_socket();
	//used to create UDP Sockets
	SOCKET initialize_udp_socket();
	//Used to TCP Bind Sockets 
	bool bind_socket(int port, std::string ipAddress, SOCKET bindingSocket);
	//Set Welcome Socket to listening mode
	bool listen_socket();

	void Print(std::string msg)
	{
		std::cout <<"DEBUG:: " <<msg << std::endl;
	}

public:
	MySocket();
	//A constructor that configures the socket and connection types, sets the IP Address and Port Number and dynamically allocates memory for the Buffer
	//(Goes to default if invalid value is given).Note that the constructor should put servers in conditions to either accept connections(if TCP), or to receive messages(if UDP).
	MySocket(SocketType sockType, std::string ipAddress, unsigned int port, ConnectionType conType, unsigned int bufSize);
	~MySocket();

	//Used to establish a TCP/IP socket connection (3-way handshake). This will return true if a successful connection is made
	bool ConnectTCP();

	//Used to disconnect an established TCP/IP socket connection (4-way handshake). Returns true/false depending on success
	bool DisconnectTCP();

	//Configures the UDP connection sockets for communication.This will return true if the sockets are successfully configured
	bool SetupUDP();

	//Used to close configured UDP sockets.Returns true / false depending on success
	bool TerminateUDP(); 

	//Used to transmit a block of RAW data, specified by the starting memory address and number of bytes over the socket 
	//and return the number of bytes transmitted. This function should work with both TCP and UDP.
	int SendData(const char* rawData, int size);

	//Used to receive the last block of RAW data stored in the internal MySocket Buffer.
	//After getting the received message into Buffer, this function will transfer its contents 
	//to the provided memory address and return the total number of bytes written. This function
	//should work with both TCP and UDP.
	int GetData(char* rawBuffer);

	//Returns the IP address configured within the MySocket object
	std::string GetIPAddr(); 

	//Changes the default IP address within the MySocket object and returns true / false depending on if successful
	bool SetIPAddr(std::string newIp); 

	//Changes the default Port number within the MySocket object and returns true / false depending on if successful
	bool SetPortNum(int newPort); 

	//Returns the Port number configured within the MySocket object
	int GetPort();

	//Returns the default SocketType the MySocket object is configured as
	SocketType GetType();

	//Changes the default SocketType within the MySocket object only if the sockets are disconnected/uninitialized. Returns true/false depending on success
	bool SetType(SocketType newSockType);
};

