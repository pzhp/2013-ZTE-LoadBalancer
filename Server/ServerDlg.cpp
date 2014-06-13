// ServerDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Server.h"
#include "ServerDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CServerDlg dialog

CServerDlg::CServerDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CServerDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CServerDlg)
	m_edit_server_id = 1;
	m_edit_server_port = 50001;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_ReportNumber = 1;
	m_bHeartReport = 1;
    m_bOnTest =0;

	m_MsgStat.correct_inrecv=0;
	m_MsgStat.recv_num=0;
	m_MsgStat.send_num=0;
	m_MsgStat.wrong_inrecv=0;
}

CServerDlg::~CServerDlg()
{
	if (m_pTimeServer != NULL)
	{
		delete m_pTimeServer;
		m_pTimeServer = NULL;
	}
}

void CServerDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CServerDlg)
	DDX_Control(pDX, IDC_LIST_MsgReport, m_listMsgReport);
	DDX_Text(pDX, IDC_EDIT_ID, m_edit_server_id);
	DDX_Text(pDX, IDC_EDIT_UDP_PORT, m_edit_server_port);
	DDV_MinMaxInt(pDX, m_edit_server_port, 1, 65535);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CServerDlg, CDialog)
	//{{AFX_MSG_MAP(CServerDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Start, OnBUTTONStart)
	ON_BN_CLICKED(IDC_BUTTON_Stop, OnBUTTONStop)
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_BUTTON_Test, OnBUTTONTest)
	ON_BN_CLICKED(IDC_BUTTON_Clear, OnBUTTONClear)
	ON_BN_CLICKED(IDC_BUTTON_Heart, OnBUTTONHeart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CServerDlg message handlers

BOOL CServerDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	

	m_listMsgReport.ModifyStyle(0,LVS_SHOWSELALWAYS);
	m_listMsgReport.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES| LVS_EX_TWOCLICKACTIVATE);
	m_listMsgReport.SetBkColor(RGB(247,247,255));
	m_listMsgReport.SetTextColor(RGB(0,0,0));
	m_listMsgReport.SetTextBkColor(RGB(247,247,255));
	m_listMsgReport.InsertColumn(0, _T("编号"), LVCFMT_CENTER, 80);//表头
	m_listMsgReport.InsertColumn(1, _T("消息类型"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(2, _T("来源ID"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(3, _T("目的ID"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(4, _T("用户ID"), LVCFMT_CENTER, 80);
    m_listMsgReport.InsertColumn(5, _T("时间信息"), LVCFMT_CENTER, 200);
	
	m_listMsgReport.DeleteAllItems();


	CString statstr;
	statstr.Format("%d条 (正确:%d 错误:%d)",m_MsgStat.recv_num, m_MsgStat.correct_inrecv, m_MsgStat.wrong_inrecv);	
	GetDlgItem(IDC_STATIC_Receive)->SetWindowText(statstr);	
	statstr.Format("%d条",m_MsgStat.send_num);
	GetDlgItem(IDC_STATIC_Send)->SetWindowText(statstr);

	//初始化报表显示功能
	GetDlgItem(IDC_BUTTON_Test)->SetWindowText("调试-关");
	GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-关");

	//构造服务器
	m_pTimeServer = new CTimeServer;
	GetDlgItem(IDC_BUTTON_Start)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_Stop)->EnableWindow(FALSE);
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CServerDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

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

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CServerDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}



void CServerDlg::OnBUTTONStart() 
{	
	UpdateData(TRUE);
	if (m_pTimeServer->StartTimeServer(m_edit_server_id,m_edit_server_port))
	{
		GetDlgItem(IDC_BUTTON_Start)->EnableWindow(FALSE);
     	GetDlgItem(IDC_BUTTON_Stop)->EnableWindow(TRUE);
	}

}

void CServerDlg::OnBUTTONStop() 
{
	m_pTimeServer->ClearUpTimeServer();
	GetDlgItem(IDC_BUTTON_Start)->EnableWindow(TRUE);
	GetDlgItem(IDC_BUTTON_Stop)->EnableWindow(FALSE);	
}

BOOL CServerDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) 
{
	
	t_msg* msg_tmp =(t_msg*)pCopyDataStruct->lpData;

	CString str, timestr,cmdstr;
				
	switch (msg_tmp->msg_type)
	{
	case CMDTYPE_TIMEREQ:
		cmdstr.Format("时间请求");		
		if (msg_tmp->dst_id == m_pTimeServer->GetServerProcID())
			m_MsgStat.correct_inrecv++;
		else
			m_MsgStat.wrong_inrecv++;
		break;
	
	case CMDTYPE_TIMEACK:
		cmdstr.Format("时间应答");
		timestr = msg_tmp->data;  //如果是时间应答，则输出时间
		m_MsgStat.send_num++;
		break;
	
	case CMDTYPE_HEARTACK:
		cmdstr.Format("心跳应答");
		m_MsgStat.send_num++;
		break;
	
	case CMDTYPE_HEARTREQ:
		cmdstr.Format("心跳请求");
		if (msg_tmp->dst_id == m_pTimeServer->GetServerProcID())
			m_MsgStat.correct_inrecv++;
		else
			m_MsgStat.wrong_inrecv++;		
		break;

	default:
		break;
	}

	if (m_bOnTest)
	{
		if( msg_tmp->msg_type== CMDTYPE_TIMEREQ || msg_tmp->msg_type== CMDTYPE_TIMEACK || m_bHeartReport)			
		{
			str.Format("%d",m_ReportNumber); 
			m_listMsgReport.InsertItem(0,str);
			m_ReportNumber++;
			
			m_listMsgReport.SetItemText(0,1,cmdstr);
			
			str.Format("%d",msg_tmp->src_id); 
			m_listMsgReport.SetItemText(0,2,str);
			
			str.Format("%d",msg_tmp->dst_id); 
			m_listMsgReport.SetItemText(0,3,str);
			
			str.Format("%d",msg_tmp->usr_id); 
			m_listMsgReport.SetItemText(0,4,str);
			
			m_listMsgReport.SetItemText(0,5,timestr);
		}
	}

	m_MsgStat.recv_num = m_MsgStat.correct_inrecv+m_MsgStat.wrong_inrecv;
	CString statstr;
	statstr.Format("%d条 (正确:%d 错误:%d)",m_MsgStat.recv_num, m_MsgStat.correct_inrecv, m_MsgStat.wrong_inrecv);	
	GetDlgItem(IDC_STATIC_Receive)->SetWindowText(statstr);

	statstr.Format("%d条",m_MsgStat.send_num);
	GetDlgItem(IDC_STATIC_Send)->SetWindowText(statstr);

	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

void CServerDlg::OnBUTTONTest() 
{
	m_bOnTest = !m_bOnTest;
	
	if (m_bOnTest)
	{	
		GetDlgItem(IDC_BUTTON_Test)->SetWindowText("调试-开"); 
		GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(TRUE);		
		m_bHeartReport = 1;
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-开"); 
		
	}
	else
	{
		GetDlgItem(IDC_BUTTON_Test)->SetWindowText("调试-关");
		GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(FALSE);
		m_bHeartReport = 0;
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-关");
	}

}

void CServerDlg::OnBUTTONClear() 
{

	m_ReportNumber = 1;
	memset(&m_MsgStat,0,sizeof(t_msg_statistics));
	m_listMsgReport.DeleteAllItems();

	//更新统计信息
	CString statstr;
	statstr.Format("%d条 (正确:%d 错误:%d)",m_MsgStat.recv_num, m_MsgStat.correct_inrecv, m_MsgStat.wrong_inrecv);	
	GetDlgItem(IDC_STATIC_Receive)->SetWindowText(statstr);	
	statstr.Format("%d条",m_MsgStat.send_num);
	GetDlgItem(IDC_STATIC_Send)->SetWindowText(statstr);
	
}

void CServerDlg::OnBUTTONHeart() 
{
	m_bHeartReport = !m_bHeartReport;
	
	if (m_bHeartReport)
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-开"); 
	else
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-关");
	
}
