// LBDlg.cpp : implementation file
//

#include "stdafx.h"
#include "LB.h"
#include "LBDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

extern CCriticalSection g_cs_msgreport;        //消息报告临界区
extern CCriticalSection g_cs_ClientAddrMag;   //管理客户端地址
/////////////////////////////////////////////////////////////////////////////
// CLBDlg dialog

CLBDlg::CLBDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CLBDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CLBDlg)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	m_bOnTest=0;
	m_ReportNumber=1;
	m_bHeartReport = 1;
}

CLBDlg::~CLBDlg()
{
	if (m_pLoadBancer != NULL)
	{
		delete m_pLoadBancer;
		m_pLoadBancer = NULL;
	}
}

void CLBDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CLBDlg)
	DDX_Control(pDX, IDC_LIST_MsgRport, m_listMsgReport);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CLBDlg, CDialog)
	//{{AFX_MSG_MAP(CLBDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_COPYDATA()
	ON_BN_CLICKED(IDC_BUTTON_Test, OnBUTTONTest)
	ON_BN_CLICKED(IDC_BUTTON_Clear, OnBUTTONClear)
	ON_BN_CLICKED(IDC_BUTTON_Heart, OnBUTTONHeart)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CLBDlg message handlers

BOOL CLBDlg::OnInitDialog()
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
    m_listMsgReport.InsertColumn(5, _T("时间信息"), LVCFMT_CENTER, 260);
	
	m_listMsgReport.DeleteAllItems();
	
	m_pLoadBancer = new CLoadBalancer;
	if(!m_pLoadBancer->StartLBServer())
	{
		AfxMessageBox("无法开启负载均衡服务,详情请查看日志文件",MB_OK);
		return 0;
	}

	//更新在线服务器数量
	CString strnum;
	strnum.Format("%d",m_pLoadBancer->GetOnlineServerNum());
	GetDlgItem(IDC_STATIC_ServerNum)->SetWindowText(strnum);

	//更新界面LB配置信息
	CString str;
	str.Format("%d",m_pLoadBancer->m_localProcID);
	GetDlgItem(IDC_STATIC_ProcID)->SetWindowText(str);
	str.Format("%d",m_pLoadBancer->m_server_udp_port);
	GetDlgItem(IDC_STATIC_ServerPort)->SetWindowText(str);
	str.Format("%d",m_pLoadBancer->m_client_udp_port);
	GetDlgItem(IDC_STATIC_ClientPort)->SetWindowText(str);

	//更新统计信息
	UpdateStatisticsInfo();
	
	//初始化报表显示功能
	GetDlgItem(IDC_BUTTON_Test)->SetWindowText("调试-关");
	GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-关");
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CLBDlg::OnPaint() 
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
HCURSOR CLBDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

BOOL CLBDlg::OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct) 
{

	if (pWnd == 0x00)
	{
		t_msg* msg_tmp =(t_msg*)pCopyDataStruct->lpData;
		
		CString str, timestr,cmdstr;
		
		if (m_bOnTest)
		{				
			switch (msg_tmp->msg_type)
			{
			case CMDTYPE_TIMEREQ:
				cmdstr.Format("时间请求");		
				break;
				
			case CMDTYPE_TIMEACK:
				cmdstr.Format("时间应答");
				timestr = msg_tmp->data;  //如果是时间应答，则输出时间
				break;
				
			case CMDTYPE_HEARTACK:
				cmdstr.Format("心跳应答");
				break;
				
			case CMDTYPE_HEARTREQ:
				cmdstr.Format("心跳请求");	
				break;
				
			default:
				break;
			}
			
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
		
		//更新统计信息
		UpdateStatisticsInfo();
	}
	else//更新服务器数量
	{
		CString strnum;
		int * num = (int*) pCopyDataStruct->lpData;
		strnum.Format("%d",*num);
		GetDlgItem(IDC_STATIC_ServerNum)->SetWindowText(strnum);
	}

	return CDialog::OnCopyData(pWnd, pCopyDataStruct);
}

void CLBDlg::OnBUTTONTest() 
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

void CLBDlg::UpdateStatisticsInfo()
{

	//更新界面统计信息
	int sendnum    = m_pLoadBancer->m_server_stat.send_num;
	int recvcorrect= m_pLoadBancer->m_server_stat.correct_inrecv;
	int recvwrong  = m_pLoadBancer->m_server_stat.wrong_inrecv;
	int recvnum = recvcorrect+recvwrong;
	
	CString statstr;
	statstr.Format("%d条 (正确:%d 错误:%d)",recvnum, recvcorrect, recvwrong);	
	GetDlgItem(IDC_STATIC_Server_Rec)->SetWindowText(statstr);
	
	statstr.Format("%d条",sendnum);
	GetDlgItem(IDC_STATIC_Server_Snd)->SetWindowText(statstr);
	
    //更新客户端套接字统计信息
	sendnum    = m_pLoadBancer->m_client_stat.send_num;
	recvcorrect= m_pLoadBancer->m_client_stat.correct_inrecv;
	recvwrong  = m_pLoadBancer->m_client_stat.wrong_inrecv;
	recvnum = recvcorrect+recvwrong;
	
	statstr.Format("%d条 (正确:%d 错误:%d)",recvnum, recvcorrect, recvwrong);	
	GetDlgItem(IDC_STATIC_Client_Rec)->SetWindowText(statstr);
	
	statstr.Format("%d条",sendnum);
	GetDlgItem(IDC_STATIC_Client_Snd)->SetWindowText(statstr);

}

void CLBDlg::OnBUTTONClear() 
{
    //list报表清理
	m_ReportNumber = 1;
	m_listMsgReport.DeleteAllItems();
    
	//清理统计信息	
	g_cs_msgreport.Lock();
	memset(&m_pLoadBancer->m_client_stat,0,sizeof(t_msg_statistics));
	memset(&m_pLoadBancer->m_server_stat,0,sizeof(t_msg_statistics));
	UpdateStatisticsInfo();
	g_cs_msgreport.Unlock();

	//清理缓存的客户端地址
	g_cs_ClientAddrMag.Lock();
	m_pLoadBancer->m_mapClientAddr.clear();
	g_cs_ClientAddrMag.Unlock();	
}

void CLBDlg::OnBUTTONHeart() 
{
	m_bHeartReport = !m_bHeartReport;
	
	if (m_bHeartReport)
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-开"); 
	else
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("心跳报表-关");
	
}
