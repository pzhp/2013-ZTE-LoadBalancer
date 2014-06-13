// LoadBalancer.cpp: implementation of the LoadBalancer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "LB.h"
#include "LoadBalancer.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

CCriticalSection g_cs_writelog;         //д��־�ٽ���   
CCriticalSection g_cs_msgreport;        //��Ϣ�����ٽ���
CCriticalSection g_cs_ServerMag;        //������������ٽ���
CCriticalSection g_cs_ClientAddrMag;    //����ͻ��˵�ַ
CCriticalSection g_cs_SessionMag;       //����Ự

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CLoadBalancer::CLoadBalancer()
{

	m_strConfigFileName = "lbconfig.ini";
	m_strLogFileName    ="LoadBalancer.log";
	m_nextselect_server  =0;

	m_localProcID = -1;

	m_client_udp_port = -1;
	m_server_udp_port = -1;
	m_client_sock = SOCKET_ERROR;
	m_server_sock = SOCKET_ERROR;

    m_ackout_run = 0;       
	m_reqin_run  = 0; 
	m_heart_run  = 0;

	memset(&m_client_stat,0,sizeof(t_msg_statistics));
    memset(&m_server_stat,0,sizeof(t_msg_statistics));

	CWnd* pWnd = AfxGetMainWnd();
	m_ui_hwnd  = pWnd->m_hWnd;

	//�������ļ��ʹ������������ļ�����д
	CString fullname = ".\\"+m_strLogFileName;
	m_ofstream.open((LPCTSTR)fullname,ios::out);
	m_ofstream <<"======================LB.exe����========================"<<endl;

	m_heartInterval   = 500;  //�������Ĭ��500ms
	m_sessionInterval = 30;   //30s
}

CLoadBalancer::~CLoadBalancer()
{
	m_heart_run  = 0;
	m_ackout_run = 0;
	m_reqin_run  = 0;
	Sleep(500); 
}

bool CLoadBalancer::ReadConfigInfo()
{
	//�����������Ϣ
	m_vecServerPoint.clear();

	//��������ļ��Ƿ����
	CFileFind ffind;
	bool bexist = ffind.FindFile(m_strConfigFileName);
	if (!bexist)
	{
		WriteLog("�����ļ�������","","");
		return 0;
	}

	//��ȡLB������Ϣ
	CString file      =".\\"+m_strConfigFileName;
	m_localProcID     = ::GetPrivateProfileInt("LoadBalancer", "process_id",-1,(LPCTSTR)file);
	m_server_udp_port =::GetPrivateProfileInt("LoadBalancer", "server_udp_port",-1,(LPCTSTR)file);
	m_client_udp_port = ::GetPrivateProfileInt("LoadBalancer", "client_udp_port",-1,(LPCTSTR)file);
	int num           = ::GetPrivateProfileInt("LoadBalancer", "server_point_number",-1,(LPCTSTR)file);   //�ڲ���Ͻ����������
	if (m_localProcID == -1 || m_server_udp_port == -1 || m_client_udp_port == -1)
	{
		WriteLog("�����ļ���ȡ����","����ID������˿ڡ��ͻ��˿ڻ��������������"," ");
		return 0;
	}

	//��ȡ�ַ�����
	int distpolicy =::GetPrivateProfileInt("LoadBalancer", "distribute_policy",-1,(LPCTSTR)file);
	if (distpolicy>2 || distpolicy < 0 )
	{
		WriteLog("�����ļ���ȡ����","LB�ַ����Բ�����������","ֻ֧��{0,1,2}");
		return 0;
	}
    m_distributePolicy = em_DistributeStrategy(distpolicy);

	//��ȡ�ػ����ֲ���
	
	int session =::GetPrivateProfileInt("LoadBalancer", "session_hold_policy",-1,(LPCTSTR)file);
	if (session>2 || session < 0  )
	{
		WriteLog("�����ļ���ȡ����","�Ự���ֹ��ܲ�����������","ֻ֧��{0,1,2}");
		return 0;
	}
	 m_sessionHold = em_SessionHold(session);

	//��ȡ��������Ϣ
	for (int i = 1; i<=num; i++)
	{
		t_server_info sertmp;
		memset(& sertmp,0,sizeof(sertmp));
	
		CString str1,str2,str3;
		str1="ServerPoint";
		str2.Format("%d",i);
		str3=str1+str2;   //�ϲ��������� 

		//��ȡ����ID
		sertmp.proc_id = ::GetPrivateProfileInt((LPCTSTR)str3, "process_id",-1,(LPCTSTR)file);

		sertmp.server_addr.sin_family = AF_INET;
		//��ȡ�˿ں�
		sertmp.server_addr.sin_port=htons(::GetPrivateProfileInt((LPCTSTR)str3, "udp_port",-1,(LPCTSTR)file));

		//��ȡIP
		char ip[20];
		::GetPrivateProfileString((LPCTSTR)str3,"ip_addr","Error",ip,20,(LPCTSTR)file);
		sertmp.server_addr.sin_addr.S_un.S_addr = inet_addr(ip);

		//�ж϶�ȡ�Ƿ�����
		if (sertmp.proc_id==-1 || sertmp.server_addr.sin_port ==-1 || strcmp(ip,"Error")==0)
		{
			WriteLog("��ȡ�����ļ�����","����˽���ID��IP����UDP�˿��д�"," ");
			return 0;
		}
		
		//��ȡȨ�ز��ж��Ƿ���[1,10]
		sertmp.ratio = (char)::GetPrivateProfileInt((LPCTSTR)str3, "ratio",-1,(LPCTSTR)file);
		if (sertmp.ratio>10 ||  sertmp.ratio<1)
		{
			WriteLog("��ȡ�����ļ�����","������Ȩ�ز�����Χ����"," ");
			return 0;
		}

		//������Ӧ�ٶ�
		sertmp.respond_speed = 0xFFFFFFFF; //�޷������ܱ�ʾ�����
		
		m_vecServerPoint.push_back(sertmp);
	}
	
	return 1;
}

bool CLoadBalancer::SetSockConfig()
{
	//�󶨷�����׽���
	
	m_server_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (m_server_sock == SOCKET_ERROR)
	{
		WriteLog("����������׽���ʧ��"," "," ");
		return 0;
	}

	SOCKADDR_IN server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family           = AF_INET;
	server_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	server_addr.sin_port             = htons(m_server_udp_port);
	
	if (-1 == bind(m_server_sock,(SOCKADDR*)&server_addr,sizeof(server_addr)))
	{
		CString str;
		str.Format("�׽��ְ󶨴������:%d, ��������������ȷ����",WSAGetLastError());
		AfxMessageBox(str,MB_OK);
		WriteLog("�󶨷�����׽���ʧ��"," "," ");
		exit(0);
	}
	
	//�󶨿ͻ����׽���

	m_client_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (m_client_sock == SOCKET_ERROR)
	{
		WriteLog("�����ͻ����׽���ʧ��"," "," ");
		return 0;
	}
	
	SOCKADDR_IN client_addr;
	memset(&client_addr,0,sizeof(client_addr));
	client_addr.sin_family           = AF_INET;
	client_addr.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	client_addr.sin_port             = htons(m_client_udp_port);
	
	if (-1 == bind(m_client_sock,(SOCKADDR*)&client_addr,sizeof(client_addr)))
	{
		CString str;
		str.Format("�׽��ְ󶨴������:%d, ��������������ȷ����",WSAGetLastError());
		AfxMessageBox(str,MB_OK);
		WriteLog("�󶨿ͻ����׽���ʧ��"," "," ");
		exit(0);
	}	

	return 1;
}

void CLoadBalancer::req_in_thread(CLoadBalancer *pLB)
{	
	fd_set fdread;
    TIMEVAL timeout;
	int iRet;

	t_msg recv_temp;  //���տͻ�������
	t_msg send_temp;  //ת�����ڲ�������
	SOCKADDR_IN client_addr;

	while (pLB->m_reqin_run)
	{
		timeout.tv_sec  = 0;
		timeout.tv_usec = 5000;
		
		FD_ZERO(&fdread);
		FD_SET(pLB->m_client_sock,&fdread);
		
		iRet = select(pLB->m_client_sock,&fdread,NULL,NULL,&timeout);
		if (0 == iRet)
		{
			continue;
		}

		if (FD_ISSET(pLB->m_client_sock,&fdread))
		{		
		//���ܿͻ��˵�����
			memset(&recv_temp,0,sizeof(recv_temp));
			memset(&client_addr,0,sizeof(client_addr));
			int addrlen = sizeof(client_addr);
			
			iRet = recvfrom(pLB->m_client_sock,(char*)&recv_temp,sizeof(recv_temp),0,(SOCKADDR*)&client_addr,&addrlen);
			int dwErr = WSAGetLastError(); 	//UDP ���͵����ɴ���������������ɽ��յ�selectΪ-1��
			if ( 10054 == dwErr)
			{
				CString str;
				str.Format("������:%d",dwErr);
				pLB->WriteLog("�ͻ��˲��ɴ�"," �ͻ���ͻȻ�ж�",str);
				continue;
			}
			if (iRet != sizeof(recv_temp))
			{
				pLB->m_reqin_run = 0;
				pLB->WriteLog("���տͻ��˵�����ʧ��"," "," ");
				break ;
			}	
			pLB->AddMsgReport(recv_temp,CLIENT_STATISTICS);
				
			//����ID��ƥ�䣬����
			if (recv_temp.dst_id != pLB->m_localProcID)
				continue;

			//ƥ��󣬽��ͻ��˵�ַ��������
			g_cs_ClientAddrMag.Lock();
			pLB->m_mapClientAddr.insert(pair<unsigned, SOCKADDR_IN>(recv_temp.usr_id,client_addr));
			g_cs_ClientAddrMag.Unlock();
		}
		
	   // �������ת��
		memcpy(&send_temp,&recv_temp,sizeof(send_temp));

		g_cs_ServerMag.Lock();
	
		g_cs_SessionMag.Lock();   //�Ự�������
		int no=pLB->SelectServerOnHoldSession(recv_temp);    	//�ڻỰ���ֻ����£�ѡ���������׼��ת��������
		g_cs_SessionMag.Unlock(); //�Ự�������
	
		if (no<0 || no >= pLB->m_vecServerPoint.size())
		{
			pLB->WriteLog("ѡȡ��������ų���","","");
			g_cs_ServerMag.Unlock();
			continue;
		}
		send_temp.dst_id   = pLB->m_vecServerPoint[no].proc_id;	
		SOCKADDR_IN saddr  = pLB->m_vecServerPoint[no].server_addr ;	
		g_cs_ServerMag.Unlock();
		TRACE("ѡ����������%d,procid %d \n",no+1,send_temp.dst_id);

		sendto(pLB->m_server_sock,(char*)&send_temp,sizeof(send_temp),0,(SOCKADDR*)&saddr,sizeof(SOCKADDR_IN));
		pLB->AddMsgReport(send_temp,SERVER_STATISTICS);
	}

}

void CLoadBalancer::ack_out_thread(CLoadBalancer *pLB)
{
	fd_set fdread;
    TIMEVAL timeout;
	int iRet;
	
	t_msg recv_temp;  //���ڲ�����������
	t_msg send_temp;  //ת�����ⲿ�ͻ���
	SOCKADDR_IN server_addr;
	
	while (pLB->m_ackout_run)
	{
		timeout.tv_sec  = 0;
		timeout.tv_usec = 5000;		
		FD_ZERO(&fdread);
		FD_SET(pLB->m_server_sock,&fdread);		
		iRet = select(pLB->m_server_sock,&fdread,NULL,NULL,&timeout);
		if (0 == iRet)
		{
			continue;
		}		
		if (FD_ISSET(pLB->m_server_sock,&fdread))
		{		
			memset(&recv_temp,0,sizeof(recv_temp));
			memset(&server_addr,0,sizeof(server_addr));
			int addrlen = sizeof(SOCKADDR_IN);

			iRet = recvfrom(pLB->m_server_sock,(char*)&recv_temp,sizeof(recv_temp),0,(SOCKADDR*)&server_addr,&addrlen);
		
			//�ų� ���򲻿ɴ�ķ������������ݣ�����׽���select����-1������ʧ�ܵ������
			int dwErr =WSAGetLastError(); //10054
			if ( 10054 == dwErr)
			{
				CString str;
				str.Format("�������:%d",dwErr);
				pLB->WriteLog("���������������ò�һ��","�з������������� ",str);
				continue;
			}
			
			if (iRet != sizeof(recv_temp))
			{
				pLB->m_ackout_run = 0;
				pLB->WriteLog("���շ�����������ʧ��","�򲻴��ڵķ��������������󣨵�ַ���˿ڲ���ȷ��"," ");
				continue;
				return ;
			}
			pLB->AddMsgReport(recv_temp, SERVER_STATISTICS);
		}
				
		//��ͻ���ת����Ϣ
		SOCKADDR_IN clientaddr;
	
		g_cs_ClientAddrMag.Lock();
		multimap<unsigned,SOCKADDR_IN>::iterator iter;
		iter = pLB->m_mapClientAddr.find(recv_temp.usr_id);  //����usr_id��Ӧ�ͻ��˵�ַ
		memcpy(&clientaddr,& iter->second, sizeof(clientaddr));
		pLB->m_mapClientAddr.erase(iter);          //ɾ����¼
		g_cs_ClientAddrMag.Unlock();

		memcpy(&send_temp,&recv_temp,sizeof(send_temp));
		send_temp.src_id   =  pLB->m_localProcID;			
		sendto(pLB->m_client_sock,(char*)&send_temp,sizeof(send_temp),0,(SOCKADDR*) &clientaddr,sizeof(SOCKADDR_IN));
		pLB->AddMsgReport(send_temp,CLIENT_STATISTICS);
		
	}

}

int CLoadBalancer::SelectServerBasedStrategy()
{
	switch (m_distributePolicy)
	{
	case Poll_Dist:
		return PollDistributeStrategy();
		
	case Ratio_Dist:
		return RatioDistributeStrategy();
		
	case Fast_Dist:
		return FastRespondDistStrategy();
		
	default:
		WriteLog("��Ч��LB�ַ�����","","");
		return -1;
	}
	
}

void CLoadBalancer::AddMsgReport( t_msg msg_tmp, int direct )
{

	g_cs_msgreport.Lock();
	//����ͳ��

	if (direct == CLIENT_STATISTICS)  // �ͻ����׽����ϵ�ͳ��
	{
		if (msg_tmp.msg_type == CMDTYPE_HEARTREQ || msg_tmp.msg_type == CMDTYPE_TIMEREQ )
		{
			if (msg_tmp.dst_id == m_localProcID)
			m_client_stat.correct_inrecv++;
			else
			m_client_stat.wrong_inrecv++;
		}
		else
		{
			m_client_stat.send_num++;
		}

	}
	else   // ������׽����ϵ�ͳ��
	{
		if (msg_tmp.msg_type == CMDTYPE_HEARTACK  || msg_tmp.msg_type == CMDTYPE_TIMEACK )
		{
			m_server_stat.correct_inrecv++;
		}
		else
		{
			m_server_stat.send_num++;
		}

	}
	g_cs_msgreport.Unlock();
	
	//���͵���������ʾ
	COPYDATASTRUCT cpytmp;
	cpytmp.dwData = 0;
	cpytmp.cbData = sizeof(msg_tmp);
	cpytmp.lpData = &msg_tmp;
	
	ASSERT(m_ui_hwnd != NULL);
	::SendMessage(m_ui_hwnd, WM_COPYDATA,(WPARAM)0,(LPARAM)&cpytmp);
	
	return;
}

bool CLoadBalancer::StartLBServer()
{
	if ( !ReadConfigInfo())
		return 0;
	SetSockConfig();

	//������������̣߳����η����汾�У�������⹦����ʱ����
	UINT ID = 0;
	m_heart_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&heart_check_thread,this,0,&ID))
	{
		m_heart_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("������������߳�ʧ��"," "," ");
		return 0;
	}
	
	//���������߳�
    ID = 0;
	m_reqin_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&req_in_thread,this,0,&ID))
	{
		m_reqin_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("���������߳�ʧ��"," "," ");
		return 0;
	}

	//���������߳�
	m_ackout_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&ack_out_thread,this,0,&ID))
	{
		m_reqin_run  = 0;
		m_ackout_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("���������߳�ʧ��"," "," ");
		return 0;
	}

	return 1;
}


void CLoadBalancer::WriteLog(CString description, CString reason, CString other)
{

	//�̰߳�ȫ
	g_cs_writelog.Lock();

	CString space="   ";

	//���ʱ��
	time_t now;
	char temp[20];
	time(&now);
    strftime(temp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
	m_ofstream<<temp<<(LPCTSTR)space;

	//�������
	m_ofstream<<(LPCTSTR)description<<(LPCTSTR)space;

	//���ԭ��
	m_ofstream<<(LPCTSTR)reason<<(LPCTSTR)space;

	//�������
	m_ofstream<<(LPCTSTR)other<<(LPCTSTR)space<<endl;

	g_cs_writelog.Unlock();

	return;
}

void CLoadBalancer::heart_check_thread(CLoadBalancer *pLB)
{
	//�������������׽���
	SOCKET heartsock = socket(AF_INET,SOCK_DGRAM,0);
	if (heartsock == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		CString  str;
		str.Format("������:%d",dwErr);
		pLB->WriteLog("������������׽���ʧ��"," ",str);
		return;
	}

	t_msg heartmsg;
	memset(&heartmsg,0,sizeof(heartmsg));

	//ѭ���������������������߷�����������4��û���յ����ж��÷�����������
	while (pLB->m_heart_run)
	{	
		g_cs_ServerMag.Lock();//���·�������Ϣʱ������߳���
	
		int num = pLB->m_vecServerPoint.size();//ӵ�����߷���������			
		for (int i=0; i<num; i++)
		{
			//�����˷�����������
			heartmsg.dst_id   = pLB->m_vecServerPoint[i].proc_id;
			heartmsg.msg_type = CMDTYPE_HEARTREQ;
			heartmsg.src_id   = pLB->m_localProcID;

			 //ȡ�÷���֮����ʱ���
			LARGE_INTEGER litmp; 
			QueryPerformanceFrequency(&litmp);
			double Freq = litmp.QuadPart;  
			QueryPerformanceCounter(&litmp);
			double QBegin = litmp.QuadPart; 
		
			//��������
			SOCKADDR_IN seradd =pLB->m_vecServerPoint[i].server_addr;
			sendto(heartsock,(char*)&heartmsg,sizeof(heartmsg),0,(SOCKADDR*)&seradd,sizeof(seradd));
			pLB->AddMsgReport(heartmsg,SERVER_STATISTICS);
		
			//���շ������������
			memset(&heartmsg,0,sizeof(heartmsg));
			SOCKADDR_IN fromaddr;
			memset(&fromaddr,0,sizeof(fromaddr));
			int len  = sizeof(fromaddr);			
			int iRet = recvfrom(heartsock,(char*)&heartmsg,sizeof(heartmsg),0,(SOCKADDR*)&fromaddr,&len);
			//�������ݵ��ж�
			DWORD dwErr = GetLastError();
			if (dwErr != 0)
			{
				pLB->m_vecServerPoint[i].unrecv_heart_count ++;
				pLB->m_vecServerPoint[i].respond_speed *= 2;
				if ( dwErr = 10054 ) // ���򲻿ɴ�ķ������������ݣ�����ʧ��
					pLB->WriteLog("������ⷢ�ַ����������ж�����","","");
				continue;
			}
			
			//ȡ�ý������ݺ��ʱ���
			QueryPerformanceCounter(&litmp);
			double QEnd = litmp.QuadPart; 
			unsigned gap_time = (unsigned)(QEnd - QBegin);     //������Ӧ�ٶȣ���Ե�λ 
		
			//TRACE("gap:%d\n",gap_time);

			//����������Ϣ��
			if (heartmsg.msg_type == CMDTYPE_HEARTACK)
			{				
			    //����ͳ����Ϣ
				pLB->AddMsgReport(heartmsg,SERVER_STATISTICS);			
				pLB->m_vecServerPoint[i].respond_speed = gap_time;  
				pLB->m_vecServerPoint[i].unrecv_heart_count = 0;
			}			
			
		}

		//���¿��÷����
		vector<t_server_info>::iterator iter;
		for (iter=pLB->m_vecServerPoint.begin();iter<pLB->m_vecServerPoint.end();iter++)
		{
			if ( iter->unrecv_heart_count >= 4)
			{	
				pLB->m_vecServerPoint.erase(iter);
			    iter--;  //����������
			}			
		}

		g_cs_ServerMag.Unlock();   //����߳���
 		
		//���͸������߳� ��ʾ��ǰ���߿��÷��������
		int number = pLB->m_vecServerPoint.size();
		COPYDATASTRUCT cpytmp;
		cpytmp.dwData = 0;
		cpytmp.cbData = sizeof(number);
		cpytmp.lpData = &number;		
		ASSERT(pLB->m_ui_hwnd != NULL);
		::SendMessage(pLB->m_ui_hwnd, WM_COPYDATA,(WPARAM)0x01,(LPARAM)&cpytmp);

		//�Ự��⣬����30s����ɾ��
		 pLB->SessionOverTimeCheck();
				
		//��������ʱ����
		Sleep(pLB->m_heartInterval);
	}

	closesocket(heartsock);
	return;
}

int CLoadBalancer::GetOnlineServerNum()
{
	return m_vecServerPoint.size();
}

int CLoadBalancer::RatioDistributeStrategy()
{
	//m_nextselect_server ��ʾ��ǰ����ѡ������
	static int ratio_count ;
	
	//��һ�ε��øú���
	static int first =1;
	if (first == 1)
	{
		ratio_count = m_vecServerPoint[m_nextselect_server].ratio;
		first--;
	}

	TRACE("ratio_count:%d\n",ratio_count);
	//�������
	if (ratio_count--)
	{
		return m_nextselect_server;
	}else
	{
		int servrnum = m_nextselect_server+1;
		m_nextselect_server = servrnum % m_vecServerPoint.size();
		ratio_count = m_vecServerPoint[m_nextselect_server].ratio;
		ratio_count--;
		return m_nextselect_server;
	}
}

int CLoadBalancer::PollDistributeStrategy()
{
	int servernum = m_nextselect_server % m_vecServerPoint.size();
	m_nextselect_server = servernum+1;     //�´�ѡ����������
	return servernum;
}

int CLoadBalancer::FastRespondDistStrategy()
{
	//ÿ�ζ����´ӷ����������У�ѡ��һ��
	int servernum = 0;  
	for (int i=1; i<m_vecServerPoint.size(); i++)
	{
		if (m_vecServerPoint[i].respond_speed < m_vecServerPoint[servernum].respond_speed )
		{
			servernum = i;
		}
	}
	return servernum;
}

int CLoadBalancer::SelectServerOnHoldSession( t_msg recv_temp )
{	
	//��ʹ�ûỰ���ֲ���
	if (m_sessionHold == No_Session)
		return SelectServerBasedStrategy();

	//ʹ�ûỰ����
	t_session_hold session ;
	memset(&session,0,sizeof(session));
	//���������ļ�ѡ��  ����src_id  ���� ����usr_id
	if (m_sessionHold == Based_Src_id)
		session.client_flag = recv_temp.src_id;
	else
		session.client_flag = recv_temp.usr_id;

	map<unsigned,t_session_hold>::iterator iter = m_mapSessionHold.find(session.client_flag);
	if (iter != m_mapSessionHold.end())
	{
		iter->second.last_time = time(NULL); //����ʱ���
		unsigned server_id = iter->second.server_id;
		
		//���ҷ��������
		TRACE("�����Ự: ");
		for (int i =0; i<m_vecServerPoint.size(); i++)
		{
			if (m_vecServerPoint[i].proc_id == server_id )
			return i;
		}
	} 
	else //û���Ѵ��ڵĻỰ���򴴽��»Ự�������䱣������m_mapSessionHold��
	{
		session.last_time = time(NULL);
		unsigned server_num = SelectServerBasedStrategy();
		session.server_id = m_vecServerPoint[server_num].proc_id;
		m_mapSessionHold.insert(pair<unsigned,t_session_hold>(session.client_flag, session));
		TRACE("�½��Ự����ǰ��ѡ��������%d  ���лỰ����%d\n",server_num,m_mapSessionHold.size());

		return server_num;		 
	}
	return -1;
}

int CLoadBalancer::SessionOverTimeCheck()
{
	g_cs_SessionMag.Lock();
	map<unsigned,t_session_hold>::iterator iter = m_mapSessionHold.begin();
    //Notice��map��ɾ��Ԫ��ʱ����ע��������仯
	for (; iter != m_mapSessionHold.end(); )
	{
		if (time(NULL)- iter->second.last_time >= m_sessionInterval) //����30�룬��ɾ���ûỰ
			m_mapSessionHold.erase(iter++);
		else
			iter++;
	}
	int session_num = m_mapSessionHold.size();
	TRACE("���лỰ��%d\n",session_num);
	g_cs_SessionMag.Unlock();
	return session_num;
}
