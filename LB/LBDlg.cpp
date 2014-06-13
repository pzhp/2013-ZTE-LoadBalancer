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

extern CCriticalSection g_cs_msgreport;        //��Ϣ�����ٽ���
extern CCriticalSection g_cs_ClientAddrMag;   //����ͻ��˵�ַ
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
	m_listMsgReport.InsertColumn(0, _T("���"), LVCFMT_CENTER, 80);//��ͷ
	m_listMsgReport.InsertColumn(1, _T("��Ϣ����"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(2, _T("��ԴID"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(3, _T("Ŀ��ID"), LVCFMT_CENTER, 80);
	m_listMsgReport.InsertColumn(4, _T("�û�ID"), LVCFMT_CENTER, 80);
    m_listMsgReport.InsertColumn(5, _T("ʱ����Ϣ"), LVCFMT_CENTER, 260);
	
	m_listMsgReport.DeleteAllItems();
	
	m_pLoadBancer = new CLoadBalancer;
	if(!m_pLoadBancer->StartLBServer())
	{
		AfxMessageBox("�޷��������ؾ������,������鿴��־�ļ�",MB_OK);
		return 0;
	}

	//�������߷���������
	CString strnum;
	strnum.Format("%d",m_pLoadBancer->GetOnlineServerNum());
	GetDlgItem(IDC_STATIC_ServerNum)->SetWindowText(strnum);

	//���½���LB������Ϣ
	CString str;
	str.Format("%d",m_pLoadBancer->m_localProcID);
	GetDlgItem(IDC_STATIC_ProcID)->SetWindowText(str);
	str.Format("%d",m_pLoadBancer->m_server_udp_port);
	GetDlgItem(IDC_STATIC_ServerPort)->SetWindowText(str);
	str.Format("%d",m_pLoadBancer->m_client_udp_port);
	GetDlgItem(IDC_STATIC_ClientPort)->SetWindowText(str);

	//����ͳ����Ϣ
	UpdateStatisticsInfo();
	
	//��ʼ��������ʾ����
	GetDlgItem(IDC_BUTTON_Test)->SetWindowText("����-��");
	GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("��������-��");
	
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
				cmdstr.Format("ʱ������");		
				break;
				
			case CMDTYPE_TIMEACK:
				cmdstr.Format("ʱ��Ӧ��");
				timestr = msg_tmp->data;  //�����ʱ��Ӧ�������ʱ��
				break;
				
			case CMDTYPE_HEARTACK:
				cmdstr.Format("����Ӧ��");
				break;
				
			case CMDTYPE_HEARTREQ:
				cmdstr.Format("��������");	
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
		
		//����ͳ����Ϣ
		UpdateStatisticsInfo();
	}
	else//���·���������
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
		GetDlgItem(IDC_BUTTON_Test)->SetWindowText("����-��"); 
		GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(TRUE);		
		m_bHeartReport = 1;
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("��������-��"); 

	}
	else
	{
		GetDlgItem(IDC_BUTTON_Test)->SetWindowText("����-��");
		GetDlgItem(IDC_BUTTON_Heart)->EnableWindow(FALSE);
		m_bHeartReport = 0;
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("��������-��");
	}
	
}

void CLBDlg::UpdateStatisticsInfo()
{

	//���½���ͳ����Ϣ
	int sendnum    = m_pLoadBancer->m_server_stat.send_num;
	int recvcorrect= m_pLoadBancer->m_server_stat.correct_inrecv;
	int recvwrong  = m_pLoadBancer->m_server_stat.wrong_inrecv;
	int recvnum = recvcorrect+recvwrong;
	
	CString statstr;
	statstr.Format("%d�� (��ȷ:%d ����:%d)",recvnum, recvcorrect, recvwrong);	
	GetDlgItem(IDC_STATIC_Server_Rec)->SetWindowText(statstr);
	
	statstr.Format("%d��",sendnum);
	GetDlgItem(IDC_STATIC_Server_Snd)->SetWindowText(statstr);
	
    //���¿ͻ����׽���ͳ����Ϣ
	sendnum    = m_pLoadBancer->m_client_stat.send_num;
	recvcorrect= m_pLoadBancer->m_client_stat.correct_inrecv;
	recvwrong  = m_pLoadBancer->m_client_stat.wrong_inrecv;
	recvnum = recvcorrect+recvwrong;
	
	statstr.Format("%d�� (��ȷ:%d ����:%d)",recvnum, recvcorrect, recvwrong);	
	GetDlgItem(IDC_STATIC_Client_Rec)->SetWindowText(statstr);
	
	statstr.Format("%d��",sendnum);
	GetDlgItem(IDC_STATIC_Client_Snd)->SetWindowText(statstr);

}

void CLBDlg::OnBUTTONClear() 
{
    //list��������
	m_ReportNumber = 1;
	m_listMsgReport.DeleteAllItems();
    
	//����ͳ����Ϣ	
	g_cs_msgreport.Lock();
	memset(&m_pLoadBancer->m_client_stat,0,sizeof(t_msg_statistics));
	memset(&m_pLoadBancer->m_server_stat,0,sizeof(t_msg_statistics));
	UpdateStatisticsInfo();
	g_cs_msgreport.Unlock();

	//������Ŀͻ��˵�ַ
	g_cs_ClientAddrMag.Lock();
	m_pLoadBancer->m_mapClientAddr.clear();
	g_cs_ClientAddrMag.Unlock();	
}

void CLBDlg::OnBUTTONHeart() 
{
	m_bHeartReport = !m_bHeartReport;
	
	if (m_bHeartReport)
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("��������-��"); 
	else
		GetDlgItem(IDC_BUTTON_Heart)->SetWindowText("��������-��");
	
}
