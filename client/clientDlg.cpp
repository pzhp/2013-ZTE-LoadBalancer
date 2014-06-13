// clientDlg.cpp : implementation file
//

#include "stdafx.h"
#include "client.h"
#include "clientDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif
;
CCriticalSection  g_cs_MsgReport ;

/////////////////////////////////////////////////////////////////////////////
// CClientDlg dialog

CClientDlg::CClientDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CClientDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CClientDlg)
	m_ReqNumber = 50;
	m_ClientProcID = 1;
	m_ServerID = 99;
	m_ClientUsrID = 2;
	m_ServerPort = 60000;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	m_ReportNumber = 1;

	m_MsgStat.correct_inrecv = 0;
	m_MsgStat.recv_num       = 0;
	m_MsgStat.send_num       = 0;
	m_MsgStat.wrong_inrecv   = 0;
}

void CClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CClientDlg)
	DDX_Control(pDX, IDC_IPADDRESS1, m_IPCtrl);
	DDX_Control(pDX, IDC_LIST_InfoReport, m_listInfoReport);
	DDX_Text(pDX, IDC_EDIT_Number, m_ReqNumber);
	DDX_Text(pDX, IDC_EDIT_ProcessID, m_ClientProcID);
	DDX_Text(pDX, IDC_EDIT_ServerID, m_ServerID);
	DDX_Text(pDX, IDC_EDIT_UsrID, m_ClientUsrID);
	DDX_Text(pDX, IDC_EDIT_ServerPort, m_ServerPort);
	DDV_MinMaxInt(pDX, m_ServerPort, 1, 65535);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CClientDlg, CDialog)
	//{{AFX_MSG_MAP(CClientDlg)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_Send, OnBUTTONSend)
	ON_BN_CLICKED(IDC_BUTTON_Clear, OnBUTTONClear)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CClientDlg message handlers

BOOL CClientDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	//�����ʼ��
	m_listInfoReport.ModifyStyle(0,LVS_SHOWSELALWAYS);
	m_listInfoReport.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES| LVS_EX_TWOCLICKACTIVATE);
	m_listInfoReport.SetBkColor(RGB(247,247,255));
	m_listInfoReport.SetTextColor(RGB(0,0,0));
	m_listInfoReport.SetTextBkColor(RGB(247,247,255));
	m_listInfoReport.InsertColumn(0, _T("���"), LVCFMT_CENTER, 80);//��ͷ
	m_listInfoReport.InsertColumn(1, _T("��Ϣ����"), LVCFMT_CENTER, 80);
	m_listInfoReport.InsertColumn(2, _T("��ԴID"), LVCFMT_CENTER, 80);
	m_listInfoReport.InsertColumn(3, _T("Ŀ��ID"), LVCFMT_CENTER, 80);
	m_listInfoReport.InsertColumn(4, _T("�û�ID"), LVCFMT_CENTER, 80);
    m_listInfoReport.InsertColumn(5, _T("ʱ����Ϣ"), LVCFMT_CENTER, 200);	
	m_listInfoReport.DeleteAllItems();

	//IP��ַ��ʼ��
	DWORD dwIP;
	dwIP = inet_addr("127.0.0.1");
	unsigned char *pIP = (unsigned char*)&dwIP;
    m_IPCtrl.SetAddress(*pIP, *(pIP+1), *(pIP+2), *(pIP+3));
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CClientDlg::OnPaint() 
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
HCURSOR CClientDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}

void CClientDlg::OnBUTTONSend() 
{
	UpdateData(TRUE);
	UINT ID=0;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&send_receive_thread,this,0,&ID))
	{
		AfxMessageBox("���������߳�ʧ��",MB_OK);
		return ;
	}
	GetDlgItem(IDC_BUTTON_Send)->EnableWindow(FALSE);
	GetDlgItem(IDC_BUTTON_Clear)->EnableWindow(FALSE);
	return ;
}

void CClientDlg::AddMsgReport(t_msg msg_tmp)
{	
	g_cs_MsgReport.Lock();  //����

	AnalyzeMsgStat(msg_tmp);  //ͳ����Ϣ
	CString str, timestr;
	//��ʾ���
	str.Format("%d",m_ReportNumber); 
	m_listInfoReport.InsertItem(0,str); 
	m_ReportNumber++;
				
	switch (msg_tmp.msg_type)
	{
	case CMDTYPE_TIMEREQ:
		str.Format("ʱ������");
		break;
	case CMDTYPE_TIMEACK:
		str.Format("ʱ��Ӧ��");
			timestr = msg_tmp.data;
		break;
	case CMDTYPE_HEARTACK:
		str.Format("����Ӧ��");
		break;
	case CMDTYPE_HEARTREQ:
		str.Format("��������");
		break;
	default:
		break;
	}
	//��ʾ��Ϣ����
	m_listInfoReport.SetItemText(0,1,str);
	//��ʾԴid			
	str.Format("%d",msg_tmp.src_id); 
	m_listInfoReport.SetItemText(0,2,str);
	//Ŀ��id			
	str.Format("%d",msg_tmp.dst_id); 
	m_listInfoReport.SetItemText(0,3,str);
	//�û�id			
	str.Format("%d",msg_tmp.usr_id); 
	m_listInfoReport.SetItemText(0,4,str);
	//ʱ����Ϣ
	m_listInfoReport.SetItemText(0,5,timestr);

	g_cs_MsgReport.Unlock();  //����
	
}

void CClientDlg::AnalyzeMsgStat(t_msg msg_tmp)
{

	if (msg_tmp.msg_type == CMDTYPE_TIMEREQ || msg_tmp.msg_type == CMDTYPE_HEARTREQ)
	{
		m_MsgStat.send_num++;
		return ;
	}
	else
	{
		if(msg_tmp.dst_id == m_ClientProcID)
			m_MsgStat.correct_inrecv++;
		else
			m_MsgStat.wrong_inrecv++;

		return ;
	}

}

void CClientDlg::send_receive_thread(CClientDlg* pclient)
{
	pclient->ClientSendAndReceive();
}

void CClientDlg::ClientSendAndReceive()
{
	//��ÿؼ��ϵ�IP
	unsigned char *pIP;
	CString strIP;
	DWORD dwIP;
	m_IPCtrl.GetAddress(dwIP);
	pIP = (unsigned char*)&dwIP;
    strIP.Format("%u.%u.%u.%u",*(pIP+3), *(pIP+2), *(pIP+1), *pIP);
	
	//������ʱ�׽���
	SOCKET sock_tmp = socket(AF_INET,SOCK_DGRAM,0);	
	if(sock_tmp == SOCKET_ERROR)
		return;
	
	//���ɶԶ˵�ַ
	SOCKADDR_IN server_addr;
	memset(&server_addr,0,sizeof(server_addr));	
	server_addr.sin_family           = AF_INET;
	server_addr.sin_addr.S_un.S_addr = inet_addr((LPCTSTR)strIP);
	server_addr.sin_port             = htons((unsigned short)m_ServerPort);
	
    //ѭ������ʱ������
	int iRet=0;
	int temp_number =m_ReqNumber; 
	TIMEVAL timeout;
	fd_set  fdread;

	while (temp_number--)
	{
		t_msg reqmsg;
		memset(&reqmsg,0,sizeof(reqmsg));
		reqmsg.dst_id = m_ServerID;
		reqmsg.src_id = m_ClientProcID;
		reqmsg.usr_id = m_ClientUsrID;
		reqmsg.msg_type =CMDTYPE_TIMEREQ;
		iRet=sendto(sock_tmp,(char*)&reqmsg,sizeof(reqmsg),0,(SOCKADDR*)&server_addr,sizeof(server_addr));
		AddMsgReport(reqmsg);    //����ͳ�����ݺ���ʾ��Ϣ����

	    //���ս��
		t_msg ackmsg;
		memset(&ackmsg,0,sizeof(ackmsg));
		int addrlen = sizeof(server_addr);
		
		timeout.tv_sec  = 1;
		timeout.tv_usec = 0;		
		FD_ZERO(&fdread);
		FD_SET(sock_tmp,&fdread);
		
		iRet = select(sock_tmp,&fdread,NULL,NULL,&timeout);
		if (0 == iRet)
		{
			continue;  //����3s�����û�����ݴﵽ����ת������һ������
		}		
		if (FD_ISSET(sock_tmp,&fdread))		
			iRet = recvfrom(sock_tmp,(char*)&ackmsg,sizeof(ackmsg),0,(SOCKADDR*)&server_addr,&addrlen);		
	
		DWORD dwErr = GetLastError();
		if (dwErr == 10054)  //�ų��򲻿ɴ�ķ������������ݵ��µĴ���
			continue;

		if (iRet != sizeof(ackmsg))
			break;
		
		AddMsgReport(ackmsg);
	}

	//�ر��׽���
	closesocket(sock_tmp);

	//�ԶԻ�����ʽ ͳ����ʾ
	CString  str;
	m_MsgStat.recv_num = m_MsgStat.correct_inrecv+m_MsgStat.wrong_inrecv;
	str.Format("������ %d ����Ϣ\n������ %d ����Ϣ (��ȷ%d��,����%d��)\n\n ȷ���˳����� ��ȷ����",
		m_MsgStat.send_num,m_MsgStat.recv_num,m_MsgStat.correct_inrecv,m_MsgStat.wrong_inrecv);
	if (AfxMessageBox(str,MB_OKCANCEL,0) == IDOK)
	{
		SendMessage(WM_CLOSE,NULL,NULL);		
	}
	else
	{
		GetDlgItem(IDC_BUTTON_Clear)->EnableWindow(TRUE);
		GetDlgItem(IDC_BUTTON_Send)->EnableWindow(TRUE);
		return;
	}
}

void CClientDlg::OnBUTTONClear() 
{
	//ɾ��������ʾ
	m_listInfoReport.DeleteAllItems();
    m_ReportNumber = 1;
	//���ͳ����Ϣ
	memset(&m_MsgStat,0,sizeof(m_MsgStat));
}
