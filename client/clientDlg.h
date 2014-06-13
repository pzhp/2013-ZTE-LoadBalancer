// clientDlg.h : header file
//

#if !defined(AFX_CLIENTDLG_H__95673841_C29E_46B5_BE5C_C8271074F2A3__INCLUDED_)
#define AFX_CLIENTDLG_H__95673841_C29E_46B5_BE5C_C8271074F2A3__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CClientDlg dialog

#include "TimeReqMsg.h"
#include <PROCESS.H>

class CClientDlg : public CDialog
{
public:
	void ClientSendAndReceive();                           //请求发送和接收，在子线程中被调用
	static void send_receive_thread(CClientDlg* pclient);  //请求发送和接收线程
	void AnalyzeMsgStat(t_msg msg_tmp);                    //消息的统计信息
	void AddMsgReport(t_msg msg_tmp);                      //调用函数先做统计，然后更新界面显示

private:
	unsigned m_ReportNumber;                               //界面显示的报表消息编号
	t_msg_statistics m_MsgStat;                            //统计消息用的对象	
	
// Construction
public:
	CClientDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	//{{AFX_DATA(CClientDlg)
	enum { IDD = IDD_CLIENT_DIALOG };
	CIPAddressCtrl	m_IPCtrl;
	CListCtrl	m_listInfoReport;
	UINT	m_ReqNumber;
	UINT	m_ClientProcID;
	UINT	m_ServerID;
	UINT	m_ClientUsrID;
	int		m_ServerPort;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClientDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CClientDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBUTTONSend();
	afx_msg void OnBUTTONClear();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIENTDLG_H__95673841_C29E_46B5_BE5C_C8271074F2A3__INCLUDED_)
