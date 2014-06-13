// LBDlg.h : header file
//

#if !defined(AFX_LBDLG_H__B7937209_7ABF_438B_9833_5442BFC220CE__INCLUDED_)
#define AFX_LBDLG_H__B7937209_7ABF_438B_9833_5442BFC220CE__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "LoadBalancer.h"

/////////////////////////////////////////////////////////////////////////////
// CLBDlg dialog

class CLBDlg : public CDialog
{
// Construction
public:
	bool m_bOnTest;                 //1表示开启调试-即在界面上显示每条消息，0表示关闭调试
	bool m_bHeartReport;            //1表示在报表中显示，0表示不在报表中显示
	int m_ReportNumber;             //界面报表显示消息详情的编号
	CLoadBalancer*  m_pLoadBancer;  // 负载均衡器主要功能类的指针
	CLBDlg(CWnd* pParent = NULL);	// standard constructor
	~CLBDlg();

// Dialog Data
	//{{AFX_DATA(CLBDlg)
	enum { IDD = IDD_LB_DIALOG };
	CListCtrl	m_listMsgReport;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CLBDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	//{{AFX_MSG(CLBDlg)
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
	afx_msg void OnBUTTONTest();
	afx_msg void OnBUTTONClear();
	afx_msg void OnTimer(UINT nIDEvent);
	afx_msg void OnBUTTONHeart();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
private:
	void UpdateStatisticsInfo();   //更新界面上的统计信息
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LBDLG_H__B7937209_7ABF_438B_9833_5442BFC220CE__INCLUDED_)
