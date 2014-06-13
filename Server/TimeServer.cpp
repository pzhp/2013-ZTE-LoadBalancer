// TimeServer.cpp: implementation of the CTimeServer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "Server.h"
#include "TimeServer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTimeServer::CTimeServer()
{

    m_thread_run    = 0;
    m_server_socket = SOCKET_ERROR;
	m_server_id     = 0;
    m_udp_port      = 0;

	CWnd* pWnd =AfxGetMainWnd();
	m_ui_hwnd = pWnd->m_hWnd;
}

CTimeServer::~CTimeServer()
{

}

bool CTimeServer::StartTimeServer(unsigned id, short port)
{
	m_server_id = id;
	m_udp_port  = port;

	//�������ݱ��׽���
	m_server_socket = socket(AF_INET,SOCK_DGRAM,0);
	if ( SOCKET_ERROR == m_server_socket )
		return 0;
	//���ɱ����׽��ֵ�ַ
	SOCKADDR_IN server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family           = AF_INET;
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	server_addr.sin_port             = htons(m_udp_port);
	//�׽������ַ��
	if (-1 == bind(m_server_socket,(SOCKADDR*)&server_addr,sizeof(server_addr)))
	{
		CString str;
		if (WSAGetLastError() == 10048 )
		{
			str.Format("����%d, �˿ڲ�����",WSAGetLastError());
		    AfxMessageBox(str,MB_OK);
		}
		return 0;
	}

	//�����շ����ݵĽ������߳�
	m_thread_run = 1;
	UINT ID=0;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&recv_thread,this,0,&ID))
	{
		closesocket(m_server_socket);
		m_thread_run = 0;
		return 0;
	}

	return 1;
}

void CTimeServer::ClearUpTimeServer()
{
	m_thread_run = 0;
}

bool CTimeServer::recv_thread(CTimeServer* pServer)
{
	fd_set fdread;
	TIMEVAL timeout;
	int iRet;

	t_msg recv_temp;
	SOCKADDR_IN client_addr;
	
	while (pServer->m_thread_run)
	{
		//selectģ�ͽ�������
		timeout.tv_sec  = 0;
		timeout.tv_usec = 5000;

		FD_ZERO(&fdread);
		FD_SET(pServer->m_server_socket,&fdread);		
		iRet = select(pServer->m_server_socket,&fdread,NULL,NULL,&timeout);
		if (0 == iRet)
			continue;

		if (FD_ISSET(pServer->m_server_socket,&fdread))
		{
			int addrlen = sizeof(client_addr);
			memset(&client_addr,0,addrlen);
			iRet = recvfrom(pServer->m_server_socket,(char*)&recv_temp,sizeof(recv_temp),0,(SOCKADDR*)&client_addr,&addrlen);
			if (iRet != sizeof(recv_temp))
			{
				pServer->m_thread_run = 0;
				break;
			}
			//ͳ������Ϣ�ı�����ʾ
			pServer->AddMsgReport(recv_temp);

			//��Ϣ��Ӧ
			t_msg send_tmp;		
			if (pServer->MsgReqAnswer(recv_temp,send_tmp)) // ʱ��������Ϣ��Ӧ,����ID����������������
			{
				iRet = sendto(pServer->m_server_socket,(char*)&send_tmp,sizeof(send_tmp),0,(SOCKADDR*)&client_addr, addrlen);
				if(iRet != sizeof(send_tmp))
				{  
					pServer->m_thread_run = 0;
					break;
				}
				//ͳ������Ϣ�ı�����ʾ
				pServer->AddMsgReport(send_tmp);
			}
		
		}		
	}
	closesocket(pServer->m_server_socket);
	return 1;
}

bool CTimeServer::MsgReqAnswer(t_msg recmsg, t_msg& senmsg)
{	
	if (recmsg.dst_id == m_server_id) // ����IDƥ��ɹ�
	{
		CString str;
		struct tm* ti ;
		time_t time_temp;
	
		//����Ӧ����Ϣ
		memcpy(&senmsg,&recmsg,sizeof(recmsg));
		senmsg.src_id = m_server_id;
		senmsg.dst_id = recmsg.src_id;

		switch (recmsg.msg_type)
		{
		case CMDTYPE_TIMEREQ :  //ʱ������ָ��
			senmsg.msg_type  = CMDTYPE_TIMEACK;
		    time_temp        = time(NULL);
			ti               = localtime(&time_temp);	
			str.Format("%d-%02d-%02d %02d:%02d:%02d",ti->tm_year+1900,ti->tm_mon+1,ti->tm_mday,ti->tm_hour,ti->tm_min,ti->tm_sec);
			strncpy(senmsg.data, (LPCTSTR)str,str.GetLength());
			break;
		case CMDTYPE_HEARTREQ : //��������ָ��
			senmsg.msg_type  = CMDTYPE_HEARTACK;
			break;
		default:
			break;
		}

		return 1;	
	}
	//����ID��ƥ�䣬�򷵻�0����������
	return 0;
}


void CTimeServer::AddMsgReport(t_msg msg_tmp)
{
	//��֯windows��Ϣ�ṹ��
	COPYDATASTRUCT cpytmp;
	cpytmp.dwData = 0;
	cpytmp.cbData = sizeof(msg_tmp);
	cpytmp.lpData = &msg_tmp;

	//������Ϣ���������߳�
	ASSERT(m_ui_hwnd != NULL);
	::SendMessage(m_ui_hwnd, WM_COPYDATA,(WPARAM)0,(LPARAM)&cpytmp);

	return;
}

UINT CTimeServer::GetServerProcID()
{
	return m_server_id;
}
