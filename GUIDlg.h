#include "MySocket.h"
#include "CmdObj.h"
#include "Log.h"
#include "BuffObj.h"
#include "Pkt_Def.h"
// GUIDlg.h : header file
//

#pragma once


// CGUIDlg dialog
class CGUIDlg : public CDialog
{
// Construction
public:
	CGUIDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_GUI_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedCancel();
	afx_msg void OnBnClickedbtnopen();
	afx_msg void UserCreated(std::string input);
private:
	CString m_ipAdd;
	int m_telePort;
	int m_cmdPort;
public:
	afx_msg void OnBnClickedbtnconnect();
	afx_msg void OnBnClickedbtnsleep();
	int m_direction;
	int m_seconds;
	afx_msg void OnBnClickedbtndrive();
	CString teleIP;
	int m_error;
};

void TelThread(std::string logname, bool & complete, std::string ipAddr, int portNum);
