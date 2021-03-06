// GUIDlg.cpp : implementation file
//

#include "stdafx.h"
#include "GUI.h"
#include "GUIDlg.h"
#include "afxdialogex.h"
#include "MySocket.h"
#include "Pkt_Def.h"
#include "CmdObj.h"
#include "BuffObj.h"
#include "Log.h"
#include "CauseErrs.h"
#include <sstream>
#include <iomanip>
#include <iostream>
#include <string>
#include <cstring>
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


bool errorState = false;
error::ErrorType err = error::ErrorType::NONERROR;

bool ExeComplete = false;
CmdObj co;
std::string logFile = "log.txt";
std::string teleFile = "telemetry.txt";

static std::string cmdLogText;
static std::string cmdRawData;

/*  hexStr code source: https://tinyurl.com/y7x32ldl */
std::string hexStr(unsigned char *data, int len)
{
	std::stringstream ss;
	ss << std::hex << std::setfill('0');
	for (int i = 0; i < len; ++i)
	{
		ss << std::setw(2) << static_cast<unsigned>(data[i]);
		ss << ", ";
	}
	return ss.str();
}

/*
* Command Thread
*/
void CommandThread(std::string logfile, bool& complete, CmdObj &co, std::string ipAddr, int portNum) {
	MySocket roboSocket(CLIENT, ipAddr, portNum, TCP, 128);

	roboSocket.ConnectTCP();

	Log log(logfile);
	std::string cmdString;									//command received from gui
	char rxBuffer[255] = { 0 };
	std::chrono::milliseconds ms(500);						//half a second for the iteration pauses
	unsigned int count = 1;									//to start the packet count

	std::string command = "UNKNOWN", extra1 = "UNKNOWN";

	do {
		PktDef transferPacket;//Create it here so that each iteration has a transferPacket that will die and call its destructor at the end
		if (!co.isEmpty()) {
			command = "UNKNOWN";
			extra1 = "UNKNOWN";
			cmdString = co.getCmd();
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
						dur = std::atoi(cmdString.c_str());
				}
			}
			else {
				command = "SLEEP";
			}

			transferPacket.SetPktCount(count);				//Sets Packet count

			if (command == "DRIVE" && dur != 0) {						//Drive command
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

			// This sets Error States and sends Data
			char* theState = errorState ? error::causeError(transferPacket, err) : transferPacket.GenPacket();
			roboSocket.SendData(theState, transferPacket.GetLength());
			roboSocket.GetData(rxBuffer);
			
			// Output data here
			cmdRawData += "TxBuffer: " + hexStr((unsigned char*)theState, transferPacket.GetLength()) + "\r\n";

			PktDef receivePacket(rxBuffer);
			cmdRawData += "RxBuffer: " + hexStr((unsigned char*)rxBuffer, receivePacket.GetLength()) + "\r\n";

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
			}

			log(s);
			cmdLogText += s + "\r\n";

			std::memset(rxBuffer, 0, 255);
		}

		std::this_thread::sleep_for(ms);

	} while (!complete || !co.isEmpty()); /* continue while not complete or still have last few commands to process, such as that last SLEEP command just added */

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
}

void TelThread(std::string logname, bool& complete, std::string ipAddr, int portNum)
{
	MySocket sock(CLIENT, ipAddr, portNum, TCP, 128);

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
	std::thread recver = std::thread(recvThread, std::ref(sock), std::ref(buff), std::ref(complete));
	while (!complete) {
		if (!buff.isEmpty()) {
			//			std::unique_ptr<char> temp = buff.get();
			BuffInfo tempBuff = buff.get();
			bool success = false;

			AllocConsole();
			freopen_s((FILE**)stdout, "CONOUT$", "w", stdout);
			std::cout << "Raw Packet: " << hexStr((unsigned char*)tempBuff.buff, tempBuff.size) << std::endl;

			if (PktDef().CheckCRC(tempBuff.buff, tempBuff.size) && tempBuff.size == STATUS_PACKET_SIZE) {
				PktDef p(tempBuff.buff);
				if (p.GetCmd() == STATUS && p.GetLength() == tempBuff.size) { /* STATUS packet, and length is the same as received length */
					success = true;
					StatusBody s;
					std::memset((char*)&s, 0, 5);
					std::memcpy((char*)&s, p.GetBodyData(), 5);
					std::string output = "";
					output += "Sonar: " + std::to_string(s.sonar) + " Arm Position: " + std::to_string(s.ArmPos);
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
						output += ", Claw achieved quantum superposition";//ROFL 

					log(output);
					std::cout << output << std::endl;

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
	sock.DisconnectTCP();
}

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CGUIDlg dialog


CGUIDlg::CGUIDlg(CWnd* pParent /*=NULL*/)
	: CDialog(IDD_GUI_DIALOG, pParent)
	, m_ipAdd(_T("127.0.0.1"))
	, m_telePort(27501)
	, m_cmdPort(27000)
	, m_direction(0)
	, m_seconds(0)
	, teleIP(_T("127.0.0.1"))
	, m_error(3)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CGUIDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, txtCmdIP, m_ipAdd);
	DDX_Text(pDX, txtTelePort, m_telePort);
	DDX_Text(pDX, txtCmdPort, m_cmdPort);
	DDX_Radio(pDX, IDC_RADIO1, m_direction);
	DDX_Radio(pDX, IDC_oneSec, m_seconds);
	DDX_Text(pDX, txtTeleIP, teleIP);
	DDX_Radio(pDX, IDC_CRC, m_error);
}

BEGIN_MESSAGE_MAP(CGUIDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDCANCEL, &CGUIDlg::OnBnClickedCancel)
	ON_BN_CLICKED(btnOpen, &CGUIDlg::OnBnClickedbtnopen)
	ON_BN_CLICKED(btnConnect, &CGUIDlg::OnBnClickedbtnconnect)
	ON_BN_CLICKED(btnSleep, &CGUIDlg::OnBnClickedbtnsleep)
	ON_BN_CLICKED(btnDrive, &CGUIDlg::OnBnClickedbtndrive)
END_MESSAGE_MAP()

std::wstring s2ws(const std::string& s) {
	int len;
	int slength = (int)s.length() + 1;
	len = MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, 0, 0);
	wchar_t* buf = new wchar_t[len];
	MultiByteToWideChar(CP_ACP, 0, s.c_str(), slength, buf, len);
	std::wstring r(buf);
	delete[] buf;
	return r;
}

// CGUIDlg message handlers

BOOL CGUIDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CGUIDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CGUIDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CGUIDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CGUIDlg::UserCreated(std::string input) {
	std::wstring stemp = s2ws(input);
	LPCWSTR result = stemp.c_str();
	SetDlgItemText(txtTelemetry, result);
}

void CGUIDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here
	CDialog::OnCancel();
}

void CGUIDlg::OnBnClickedbtnopen()
{
	// TODO: Add your control notification handler code here

}

void CGUIDlg::OnBnClickedbtnconnect()
{
	// Updates variables to latest values
	UpdateData(TRUE);

	std::string ipAd2(CW2A(m_ipAdd.GetString(), CP_UTF8));

	// Create Threads
	std::thread cmdThd(CommandThread, logFile, std::ref(ExeComplete), std::ref(co), ipAd2, m_cmdPort);
	cmdThd.detach();
	std::thread telThd(TelThread, teleFile, std::ref(ExeComplete), ipAd2, m_telePort);
	telThd.detach();
}


void CGUIDlg::OnBnClickedbtnsleep()
{
	// Sets To Sleep
	co.addCmd("SLEEP");

	ExeComplete = true;
	Sleep(3000);
}


void CGUIDlg::OnBnClickedbtndrive()
{
	UpdateData(TRUE);

	std::string ACTION;
	std::string DIRECTION;
	std::string COMMAND = "A";

	// check if in error state
	if (m_error < 3) 
		errorState = true;
	else 
		errorState = false;

	err = (error::ErrorType)m_error;
	m_seconds++; // radio buttons start at 0
	if (m_direction == 0) {
		ACTION = "DRIVE";
		DIRECTION = "FORWARD";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + " for " + std::to_string(m_seconds) + " seconds\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 1) {
		ACTION = "DRIVE";
		DIRECTION = "BACKWARD";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);
		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + " for " + std::to_string(m_seconds) + " seconds\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 2) {
		ACTION = "DRIVE";
		DIRECTION = "LEFT";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + " for " + std::to_string(m_seconds) + " seconds\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 3) {
		ACTION = "DRIVE";
		DIRECTION = "RIGHT";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + " for " + std::to_string(m_seconds) + " seconds\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 4) {
		ACTION = "ARM";
		DIRECTION = "UP";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + "\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 5) {
		ACTION = "ARM";
		DIRECTION = "DOWN";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + "\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 6) {
		ACTION = "CLAW";
		DIRECTION = "OPEN";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + "\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}
	else if (m_direction == 7) {
		ACTION = "CLAW";
		DIRECTION = "CLOSE";
		COMMAND = ACTION + "," + DIRECTION + "," + std::to_string(m_seconds);
		co.addCmd(COMMAND);

		cmdLogText += "Command sent " + ACTION + " " + DIRECTION + "\r\n";
		std::wstring stemp = s2ws(cmdLogText);
		LPCWSTR result = stemp.c_str();
		SetDlgItemText(txtLog, result);
	}

	std::wstring atemp = s2ws(cmdRawData);
	LPCWSTR aresult = atemp.c_str();
	SetDlgItemText(txtRawOut, aresult);
}
