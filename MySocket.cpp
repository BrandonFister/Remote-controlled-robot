#include "MySocket.h"

//TODO: Create WSA Destructor (close the socket)


bool MySocket::StartWSA()
{
	if ((WSAStartup(MAKEWORD(1, 0), &this->wsa_data)) != 0) {
		return false;
	}
	return true;
}

MySocket::MySocket()
{
	this->m_bConnect = false;
	this->m_Buffer = nullptr;
	this->m_MaxSize = DEFAULT_SIZE;
	this->m_Port = -1;
}

MySocket::MySocket(SocketType sockType, std::string ipAddress, unsigned int port, ConnectionType conType, unsigned int bufSize)
{
	this->m_MaxSize = DEFAULT_SIZE;
	this->m_mySocket = sockType;
	this->m_connectionType = conType;
	this->m_Port = port;
	this->m_IPAddr = ipAddress;

	if (bufSize > DEFAULT_SIZE || bufSize < 0)
		this->m_Buffer = new char[DEFAULT_SIZE];
	else
		this->m_Buffer = new char[bufSize];


	if (!this->StartWSA())//check that WSA starts first
	{
		Print("MySocket could not initialize server");
	}
}


MySocket::~MySocket()
{
	if (this->m_Buffer != nullptr)
		delete[] m_Buffer;

	m_Buffer = nullptr;
	WSACleanup();
}

bool MySocket::ConnectTCP()
{
	//HINT: This function should have logic to prevent a TCP being configured as a UDP and visa-versa

	if (this->m_connectionType == ConnectionType::TCP)
	{
		if (this->m_mySocket == SocketType::SERVER)//check if it is a server
		{
			if (this->m_connectionType == ConnectionType::TCP)
			{
				this->m_WelcomeSocket = this->initialize_tcp_socket();//Generates the welcome socket
				this->m_bConnect = this->bind_socket(this->m_Port, this->m_IPAddr, this->m_WelcomeSocket);//binds the welcome socket
				this->m_bConnect = this->listen_socket();//sets the welcome socket to listen
				if ((this->m_ConnectionSocket = accept(this->m_WelcomeSocket, NULL, NULL)) == SOCKET_ERROR)
				{
					Print("Could not accept incoming connection");
				}
				else
				{
					return this->m_bConnect;
				}
			}
			else
			{
				Print("Critical Error: Not a valid connection type cannot start server!");
			}
		}
		else
		{
			this->m_ConnectionSocket = this->initialize_tcp_socket();

			struct sockaddr_in SvrAddr;
			SvrAddr.sin_family = AF_INET; //Address family type Internet
			SvrAddr.sin_port = htons(this->m_Port); //port (host to network conversion)
			SvrAddr.sin_addr.s_addr = inet_addr(this->m_IPAddr.c_str()); //IP address
			if ((::connect(this->m_ConnectionSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
				closesocket(this->m_ConnectionSocket);
				Print("Could not connect to the TCP server");
			}
			else
			{
				this->m_bConnect = true;
				return true;//only client success will get here 
			}
		}
	}
	return false;//all failures will fall through to here
}

bool MySocket::DisconnectTCP()
{
	//HINT: This function should have logic to prevent a TCP being configured as a UDP and visa-versa
	if (this->m_connectionType == ConnectionType::TCP)
	{
		this->m_bConnect = false;
		closesocket(this->m_ConnectionSocket);
		return true;
	}
	return false;
}

bool MySocket::SetupUDP()
{

	if (this->m_connectionType == ConnectionType::UDP) { // check if correct connection type

		if (this->m_mySocket == SocketType::SERVER)
		{
			this->m_ConnectionSocket = this->initialize_udp_socket();
			struct sockaddr_in SrvAddr;
			SrvAddr.sin_family = AF_INET;
			SrvAddr.sin_port = htons(this->m_Port);
			SrvAddr.sin_addr.s_addr = INADDR_ANY;
			if (bind(this->m_ConnectionSocket, (struct sockaddr *)&SrvAddr, sizeof(SrvAddr)) == SOCKET_ERROR)
			{
				closesocket(this->m_ConnectionSocket);
			}
			else
			{
				this->m_bConnect = true;
				return true;
			}
		}
		else
		{
			SOCKET LocalSocket = this->initialize_udp_socket();

			if (LocalSocket == INVALID_SOCKET) { // if the socket is invalid, shut down
				Print("Could not initialize UDP socket!");
			}
			else { // if socket is valid, assign socket to current object socket
				m_ConnectionSocket = LocalSocket;
				struct sockaddr_in SvrAddr;
				SvrAddr.sin_addr.s_addr = inet_addr(this->m_IPAddr.c_str());
				SvrAddr.sin_port = htons(this->m_Port);
				SvrAddr.sin_family = AF_INET;

				this->m_RespAddr = SvrAddr;
				this->m_bConnect = true;
				return true;
			}
		}
	}
	return false;
}

bool MySocket::TerminateUDP()
{	// confirms connection is a UDP connection or 
	// does nothing and returns false
	if (this->m_connectionType == ConnectionType::UDP) {
		closesocket(m_ConnectionSocket);
		this->m_bConnect = false;
		return true;
	}
	else {
		return false;
	}
}

int MySocket::SendData(const char * rawData, int size)
{
	if (m_connectionType == TCP)
		size = send(m_ConnectionSocket, rawData, size, 0);
	else if (m_connectionType == UDP)
		size = sendto(m_ConnectionSocket, rawData, size, 0, (const sockaddr *)&m_RespAddr, sizeof(this->m_RespAddr));
	else
		throw("ERROR: Unknown connection type");
	return size;
}

int MySocket::GetData(char * rawBuffer)
{
	int addessSize = sizeof(this->m_RespAddr);
	int size = 0;
	if (m_connectionType == TCP)
		size = recv(m_ConnectionSocket, rawBuffer, m_MaxSize, 0);
	else if (m_connectionType == UDP)
		size = recvfrom(m_ConnectionSocket, rawBuffer, m_MaxSize, 0, (sockaddr *)&m_RespAddr, &addessSize);
	else
		throw("ERROR: Unknown connection type");
	return size;
}

std::string MySocket::GetIPAddr()
{
	if (m_IPAddr.empty()) {
		return "";
	}
	else {
		return this->m_IPAddr;
	}
}

bool MySocket::SetIPAddr(std::string newIp)
{
	//method should return an error if a connection has already been established
	//if (m_IPAddr.empty() && m_bConnect==false) {
	if (!m_bConnect) {
		m_IPAddr = newIp;
		return true;
	}
	else {
		return false;
	}
}

bool MySocket::SetPortNum(int newPort)
{
	if (m_bConnect == true) {
		return false;
	}
	else {
		m_Port = newPort;
		if (m_Port == newPort)
			return true;
		else
			return false;
	}
	//return isTrue;
}

int MySocket::GetPort()
{
	return this->m_Port;
}

SocketType MySocket::GetType()
{
	return this->m_mySocket;
}

bool MySocket::SetType(SocketType newSockType)
{
	if (m_bConnect == false) {
		this->m_mySocket = newSockType;
		return true;
	}
	else
		return false;
	//HINT: Set functionality should contain logic to prevent the header information from being changed if a TCP/IP connection is established or a UDP Server is bound to a port number
}

SOCKET MySocket::initialize_tcp_socket() {
	SOCKET LocalSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (LocalSocket == INVALID_SOCKET) {
		Print("Could not initialize TCP socket! CRITICAL ERROR");
	}
	return LocalSocket;
}

bool MySocket::bind_socket(int port, std::string ipAddress, SOCKET bindingSocket) {
	struct sockaddr_in SvrAddr;
	SvrAddr.sin_family = AF_INET; //Address family type internet
	SvrAddr.sin_port = htons(port); //port (host to network conversion)
	SvrAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str()); //IP address
	if ((::bind(bindingSocket, (struct sockaddr *)&SvrAddr, sizeof(SvrAddr))) == SOCKET_ERROR) {
		closesocket(bindingSocket);
		Print("Could not bind to the socket");
		return false;
	}
	else
	{
		Print("Socket Bound");
		return true;
	}
}

bool MySocket::listen_socket() {
	if (listen(this->m_WelcomeSocket, 1) == SOCKET_ERROR) {
		closesocket(this->m_WelcomeSocket);
		Print("Could not listen to the provided socket.");
		return false;
	}
	else {
		Print("Welcome Socket Listening");
		return true;
	}
}

SOCKET MySocket::initialize_udp_socket() {
	SOCKET LocalSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (LocalSocket == INVALID_SOCKET) {
		Print("Could not initialize socket");
	}
	return LocalSocket;
}

