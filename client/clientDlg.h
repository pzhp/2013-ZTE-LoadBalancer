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
	void ClientSendAndReceive();                           //�����ͺͽ��գ������߳��б�����
	static void send_receive_thread(CClientDlg* pclient);  //�����ͺͽ����߳�
	void AnalyzeMsgStat(t_msg msg_tmp);                    //��Ϣ��ͳ����Ϣ
	void AddMsgReport(t_msg msg_tmp);                      //���ú�������ͳ�ƣ�Ȼ����½�����ʾ

private:
	unsigned m_ReportNumber;                               //������ʾ�ı�����Ϣ���
	t_msg_statistics m_MsgStat;                            //ͳ����Ϣ�õĶ���	
	
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
