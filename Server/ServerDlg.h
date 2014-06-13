// ServerDlg.h : header file
//

#if !defined(AFX_SERVERDLG_H__3815C7A4_9319_41DB_B637_49C4334BB32B__INCLUDED_)
#define AFX_SERVERDLG_H__3815C7A4_9319_41DB_B637_49C4334BB32B__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

/////////////////////////////////////////////////////////////////////////////
// CServerDlg dialog

#include "TimeServer.h"

class CServerDlg : public CDialog
{

private:
	UINT m_ReportNumber;                  // 消息报表显示编号
	bool m_bOnTest;                       // 调试开关是否打开，1是打开显示消息详情，0关闭表示不显示
	bool m_bHeartReport;                  // 1在报表中显示心跳信息，0表示不显示
	t_msg_statistics m_MsgStat;           // 信息的统计	
	CTimeServer* m_pTimeServer;           // 时间服务器实例化指针
	
// Construction
public:
	CServerDlg(CWnd* pParent = NULL);	 // standard constructor
	~CServerDlg();

// Dialog Data
	//{{AFX_DATA(CServerDlg)
	enum { IDD = IDD_SERVER_DIALOG };
	CListCtrl	m_listMsgReport;
	UINT	m_edit_server_id;
	int		m_edit_server_port;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CServerDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CServerDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg void OnBUTTONStart();
	afx_msg void OnBUTTONStop();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnBUTTONTest();
	afx_msg void OnBUTTONClear();
	afx_msg void OnBUTTONHeart();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SERVERDLG_H__3815C7A4_9319_41DB_B637_49C4334BB32B__INCLUDED_)
