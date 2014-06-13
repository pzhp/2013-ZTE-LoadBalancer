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

CCriticalSection g_cs_writelog;         //写日志临界区   
CCriticalSection g_cs_msgreport;        //消息报告临界区
CCriticalSection g_cs_ServerMag;        //管理服务器的临界区
CCriticalSection g_cs_ClientAddrMag;    //管理客户端地址
CCriticalSection g_cs_SessionMag;       //管理会话

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

	//不存在文件就创建，否则在文件后面写
	CString fullname = ".\\"+m_strLogFileName;
	m_ofstream.open((LPCTSTR)fullname,ios::out);
	m_ofstream <<"======================LB.exe启动========================"<<endl;

	m_heartInterval   = 500;  //心跳间隔默认500ms
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
	//清除服务器信息
	m_vecServerPoint.clear();

	//检查配置文件是否存在
	CFileFind ffind;
	bool bexist = ffind.FindFile(m_strConfigFileName);
	if (!bexist)
	{
		WriteLog("配置文件不存在","","");
		return 0;
	}

	//读取LB配置信息
	CString file      =".\\"+m_strConfigFileName;
	m_localProcID     = ::GetPrivateProfileInt("LoadBalancer", "process_id",-1,(LPCTSTR)file);
	m_server_udp_port =::GetPrivateProfileInt("LoadBalancer", "server_udp_port",-1,(LPCTSTR)file);
	m_client_udp_port = ::GetPrivateProfileInt("LoadBalancer", "client_udp_port",-1,(LPCTSTR)file);
	int num           = ::GetPrivateProfileInt("LoadBalancer", "server_point_number",-1,(LPCTSTR)file);   //内部管辖服务器个数
	if (m_localProcID == -1 || m_server_udp_port == -1 || m_client_udp_port == -1)
	{
		WriteLog("配置文件读取有误","进程ID、服务端口、客户端口或服务器数量不对"," ");
		return 0;
	}

	//读取分发策略
	int distpolicy =::GetPrivateProfileInt("LoadBalancer", "distribute_policy",-1,(LPCTSTR)file);
	if (distpolicy>2 || distpolicy < 0 )
	{
		WriteLog("配置文件读取有误","LB分发策略参数配置有误","只支持{0,1,2}");
		return 0;
	}
    m_distributePolicy = em_DistributeStrategy(distpolicy);

	//读取回话保持策略
	
	int session =::GetPrivateProfileInt("LoadBalancer", "session_hold_policy",-1,(LPCTSTR)file);
	if (session>2 || session < 0  )
	{
		WriteLog("配置文件读取有误","会话保持功能参数配置有误","只支持{0,1,2}");
		return 0;
	}
	 m_sessionHold = em_SessionHold(session);

	//读取服务器信息
	for (int i = 1; i<=num; i++)
	{
		t_server_info sertmp;
		memset(& sertmp,0,sizeof(sertmp));
	
		CString str1,str2,str3;
		str1="ServerPoint";
		str2.Format("%d",i);
		str3=str1+str2;   //合并后代表键名 

		//读取进程ID
		sertmp.proc_id = ::GetPrivateProfileInt((LPCTSTR)str3, "process_id",-1,(LPCTSTR)file);

		sertmp.server_addr.sin_family = AF_INET;
		//读取端口号
		sertmp.server_addr.sin_port=htons(::GetPrivateProfileInt((LPCTSTR)str3, "udp_port",-1,(LPCTSTR)file));

		//读取IP
		char ip[20];
		::GetPrivateProfileString((LPCTSTR)str3,"ip_addr","Error",ip,20,(LPCTSTR)file);
		sertmp.server_addr.sin_addr.S_un.S_addr = inet_addr(ip);

		//判断读取是否正常
		if (sertmp.proc_id==-1 || sertmp.server_addr.sin_port ==-1 || strcmp(ip,"Error")==0)
		{
			WriteLog("读取配置文件有误","服务端进程ID、IP或者UDP端口有错"," ");
			return 0;
		}
		
		//读取权重并判断是否在[1,10]
		sertmp.ratio = (char)::GetPrivateProfileInt((LPCTSTR)str3, "ratio",-1,(LPCTSTR)file);
		if (sertmp.ratio>10 ||  sertmp.ratio<1)
		{
			WriteLog("读取配置文件有误","服务器权重参数范围有误"," ");
			return 0;
		}

		//设置响应速度
		sertmp.respond_speed = 0xFFFFFFFF; //无符号所能表示的最大
		
		m_vecServerPoint.push_back(sertmp);
	}
	
	return 1;
}

bool CLoadBalancer::SetSockConfig()
{
	//绑定服务端套接字
	
	m_server_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (m_server_sock == SOCKET_ERROR)
	{
		WriteLog("创建服务端套接字失败"," "," ");
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
		str.Format("套接字绑定错误代码:%d, 结束程序请点击“确定”",WSAGetLastError());
		AfxMessageBox(str,MB_OK);
		WriteLog("绑定服务端套接字失败"," "," ");
		exit(0);
	}
	
	//绑定客户端套接字

	m_client_sock = socket(AF_INET,SOCK_DGRAM,0);
	if (m_client_sock == SOCKET_ERROR)
	{
		WriteLog("创建客户端套接字失败"," "," ");
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
		str.Format("套接字绑定错误代码:%d, 结束程序请点击“确定”",WSAGetLastError());
		AfxMessageBox(str,MB_OK);
		WriteLog("绑定客户端套接字失败"," "," ");
		exit(0);
	}	

	return 1;
}

void CLoadBalancer::req_in_thread(CLoadBalancer *pLB)
{	
	fd_set fdread;
    TIMEVAL timeout;
	int iRet;

	t_msg recv_temp;  //接收客户端请求
	t_msg send_temp;  //转发给内部服务器
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
		//接受客户端的数据
			memset(&recv_temp,0,sizeof(recv_temp));
			memset(&client_addr,0,sizeof(client_addr));
			int addrlen = sizeof(client_addr);
			
			iRet = recvfrom(pLB->m_client_sock,(char*)&recv_temp,sizeof(recv_temp),0,(SOCKADDR*)&client_addr,&addrlen);
			int dwErr = WSAGetLastError(); 	//UDP 发送到不可达主机，反而会造成接收的select为-1；
			if ( 10054 == dwErr)
			{
				CString str;
				str.Format("错误码:%d",dwErr);
				pLB->WriteLog("客户端不可达"," 客户端突然中断",str);
				continue;
			}
			if (iRet != sizeof(recv_temp))
			{
				pLB->m_reqin_run = 0;
				pLB->WriteLog("接收客户端的数据失败"," "," ");
				break ;
			}	
			pLB->AddMsgReport(recv_temp,CLIENT_STATISTICS);
				
			//进程ID不匹配，丢弃
			if (recv_temp.dst_id != pLB->m_localProcID)
				continue;

			//匹配后，将客户端地址缓存起来
			g_cs_ClientAddrMag.Lock();
			pLB->m_mapClientAddr.insert(pair<unsigned, SOCKADDR_IN>(recv_temp.usr_id,client_addr));
			g_cs_ClientAddrMag.Unlock();
		}
		
	   // 向服务器转发
		memcpy(&send_temp,&recv_temp,sizeof(send_temp));

		g_cs_ServerMag.Lock();
	
		g_cs_SessionMag.Lock();   //会话管理加锁
		int no=pLB->SelectServerOnHoldSession(recv_temp);    	//在会话保持基础下，选择服务器，准备转发的数据
		g_cs_SessionMag.Unlock(); //会话管理解锁
	
		if (no<0 || no >= pLB->m_vecServerPoint.size())
		{
			pLB->WriteLog("选取服务器编号出错","","");
			g_cs_ServerMag.Unlock();
			continue;
		}
		send_temp.dst_id   = pLB->m_vecServerPoint[no].proc_id;	
		SOCKADDR_IN saddr  = pLB->m_vecServerPoint[no].server_addr ;	
		g_cs_ServerMag.Unlock();
		TRACE("选择服务器编号%d,procid %d \n",no+1,send_temp.dst_id);

		sendto(pLB->m_server_sock,(char*)&send_temp,sizeof(send_temp),0,(SOCKADDR*)&saddr,sizeof(SOCKADDR_IN));
		pLB->AddMsgReport(send_temp,SERVER_STATISTICS);
	}

}

void CLoadBalancer::ack_out_thread(CLoadBalancer *pLB)
{
	fd_set fdread;
    TIMEVAL timeout;
	int iRet;
	
	t_msg recv_temp;  //从内部服务器接收
	t_msg send_temp;  //转发给外部客户端
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
		
			//排除 因向不可达的服务器发送数据，造成套接字select返回-1、接受失败的情况。
			int dwErr =WSAGetLastError(); //10054
			if ( 10054 == dwErr)
			{
				CString str;
				str.Format("错误代号:%d",dwErr);
				pLB->WriteLog("服务器数量与配置不一样","有服务器结束服务 ",str);
				continue;
			}
			
			if (iRet != sizeof(recv_temp))
			{
				pLB->m_ackout_run = 0;
				pLB->WriteLog("接收服务器的数据失败","向不存在的服务器发送了请求（地址、端口不正确）"," ");
				continue;
				return ;
			}
			pLB->AddMsgReport(recv_temp, SERVER_STATISTICS);
		}
				
		//向客户端转发信息
		SOCKADDR_IN clientaddr;
	
		g_cs_ClientAddrMag.Lock();
		multimap<unsigned,SOCKADDR_IN>::iterator iter;
		iter = pLB->m_mapClientAddr.find(recv_temp.usr_id);  //查找usr_id对应客户端地址
		memcpy(&clientaddr,& iter->second, sizeof(clientaddr));
		pLB->m_mapClientAddr.erase(iter);          //删除记录
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
		WriteLog("无效的LB分发策略","","");
		return -1;
	}
	
}

void CLoadBalancer::AddMsgReport( t_msg msg_tmp, int direct )
{

	g_cs_msgreport.Lock();
	//先做统计

	if (direct == CLIENT_STATISTICS)  // 客户端套接字上的统计
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
	else   // 服务端套接字上的统计
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
	
	//发送到界面上显示
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

	//开启心跳检测线程，本次发布版本中，心跳检测功能暂时屏蔽
	UINT ID = 0;
	m_heart_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&heart_check_thread,this,0,&ID))
	{
		m_heart_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("开启心跳检测线程失败"," "," ");
		return 0;
	}
	
	//开启接收线程
    ID = 0;
	m_reqin_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&req_in_thread,this,0,&ID))
	{
		m_reqin_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("开启接收线程失败"," "," ");
		return 0;
	}

	//开启发送线程
	m_ackout_run = 1;
	if(NULL==_beginthreadex(0,1024*256,(unsigned (__stdcall*)(void *))&ack_out_thread,this,0,&ID))
	{
		m_reqin_run  = 0;
		m_ackout_run = 0;
		closesocket(m_client_sock);
		closesocket(m_server_sock);
		WriteLog("开启发送线程失败"," "," ");
		return 0;
	}

	return 1;
}


void CLoadBalancer::WriteLog(CString description, CString reason, CString other)
{

	//线程安全
	g_cs_writelog.Lock();

	CString space="   ";

	//输出时间
	time_t now;
	char temp[20];
	time(&now);
    strftime(temp, 20, "%Y-%m-%d %H:%M:%S", localtime(&now));
	m_ofstream<<temp<<(LPCTSTR)space;

	//输出描述
	m_ofstream<<(LPCTSTR)description<<(LPCTSTR)space;

	//输出原因
	m_ofstream<<(LPCTSTR)reason<<(LPCTSTR)space;

	//输出其他
	m_ofstream<<(LPCTSTR)other<<(LPCTSTR)space<<endl;

	g_cs_writelog.Unlock();

	return;
}

void CLoadBalancer::heart_check_thread(CLoadBalancer *pLB)
{
	//创建心跳检测的套接字
	SOCKET heartsock = socket(AF_INET,SOCK_DGRAM,0);
	if (heartsock == SOCKET_ERROR)
	{
		DWORD dwErr = WSAGetLastError();
		CString  str;
		str.Format("错误码:%d",dwErr);
		pLB->WriteLog("创建心跳检测套接字失败"," ",str);
		return;
	}

	t_msg heartmsg;
	memset(&heartmsg,0,sizeof(heartmsg));

	//循环发送心跳包，更新在线服务器，连续4次没有收到则判定该服务器不可用
	while (pLB->m_heart_run)
	{	
		g_cs_ServerMag.Lock();//更新服务器信息时，需加线程锁
	
		int num = pLB->m_vecServerPoint.size();//拥有在线服务器数量			
		for (int i=0; i<num; i++)
		{
			//向服务端发送心跳请求
			heartmsg.dst_id   = pLB->m_vecServerPoint[i].proc_id;
			heartmsg.msg_type = CMDTYPE_HEARTREQ;
			heartmsg.src_id   = pLB->m_localProcID;

			 //取得发送之初的时间戳
			LARGE_INTEGER litmp; 
			QueryPerformanceFrequency(&litmp);
			double Freq = litmp.QuadPart;  
			QueryPerformanceCounter(&litmp);
			double QBegin = litmp.QuadPart; 
		
			//发送数据
			SOCKADDR_IN seradd =pLB->m_vecServerPoint[i].server_addr;
			sendto(heartsock,(char*)&heartmsg,sizeof(heartmsg),0,(SOCKADDR*)&seradd,sizeof(seradd));
			pLB->AddMsgReport(heartmsg,SERVER_STATISTICS);
		
			//接收服务端心跳数据
			memset(&heartmsg,0,sizeof(heartmsg));
			SOCKADDR_IN fromaddr;
			memset(&fromaddr,0,sizeof(fromaddr));
			int len  = sizeof(fromaddr);			
			int iRet = recvfrom(heartsock,(char*)&heartmsg,sizeof(heartmsg),0,(SOCKADDR*)&fromaddr,&len);
			//接受数据的判定
			DWORD dwErr = GetLastError();
			if (dwErr != 0)
			{
				pLB->m_vecServerPoint[i].unrecv_heart_count ++;
				pLB->m_vecServerPoint[i].respond_speed *= 2;
				if ( dwErr = 10054 ) // 因向不可达的服务器发送数据，导致失败
					pLB->WriteLog("心跳检测发现服务器可能中断连接","","");
				continue;
			}
			
			//取得接受数据后的时间戳
			QueryPerformanceCounter(&litmp);
			double QEnd = litmp.QuadPart; 
			unsigned gap_time = (unsigned)(QEnd - QBegin);     //量化响应速度，相对单位 
		
			//TRACE("gap:%d\n",gap_time);

			//更新心跳信息：
			if (heartmsg.msg_type == CMDTYPE_HEARTACK)
			{				
			    //更新统计信息
				pLB->AddMsgReport(heartmsg,SERVER_STATISTICS);			
				pLB->m_vecServerPoint[i].respond_speed = gap_time;  
				pLB->m_vecServerPoint[i].unrecv_heart_count = 0;
			}			
			
		}

		//更新可用服务端
		vector<t_server_info>::iterator iter;
		for (iter=pLB->m_vecServerPoint.begin();iter<pLB->m_vecServerPoint.end();iter++)
		{
			if ( iter->unrecv_heart_count >= 4)
			{	
				pLB->m_vecServerPoint.erase(iter);
			    iter--;  //迭代器回退
			}			
		}

		g_cs_ServerMag.Unlock();   //解除线程锁
 		
		//发送给界面线程 显示当前在线可用服务端数量
		int number = pLB->m_vecServerPoint.size();
		COPYDATASTRUCT cpytmp;
		cpytmp.dwData = 0;
		cpytmp.cbData = sizeof(number);
		cpytmp.lpData = &number;		
		ASSERT(pLB->m_ui_hwnd != NULL);
		::SendMessage(pLB->m_ui_hwnd, WM_COPYDATA,(WPARAM)0x01,(LPARAM)&cpytmp);

		//会话检测，超过30s，则删除
		 pLB->SessionOverTimeCheck();
				
		//发送心跳时间间隔
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
	//m_nextselect_server 表示当前的所选服务器
	static int ratio_count ;
	
	//第一次调用该函数
	static int first =1;
	if (first == 1)
	{
		ratio_count = m_vecServerPoint[m_nextselect_server].ratio;
		first--;
	}

	TRACE("ratio_count:%d\n",ratio_count);
	//分配策略
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
	m_nextselect_server = servernum+1;     //下次选择服务器编号
	return servernum;
}

int CLoadBalancer::FastRespondDistStrategy()
{
	//每次都重新从服务器向量中，选择一个
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
	//不使用会话保持策略
	if (m_sessionHold == No_Session)
		return SelectServerBasedStrategy();

	//使用会话保持
	t_session_hold session ;
	memset(&session,0,sizeof(session));
	//根据配置文件选择  基于src_id  还是 基于usr_id
	if (m_sessionHold == Based_Src_id)
		session.client_flag = recv_temp.src_id;
	else
		session.client_flag = recv_temp.usr_id;

	map<unsigned,t_session_hold>::iterator iter = m_mapSessionHold.find(session.client_flag);
	if (iter != m_mapSessionHold.end())
	{
		iter->second.last_time = time(NULL); //更新时间戳
		unsigned server_id = iter->second.server_id;
		
		//查找服务器编号
		TRACE("继续会话: ");
		for (int i =0; i<m_vecServerPoint.size(); i++)
		{
			if (m_vecServerPoint[i].proc_id == server_id )
			return i;
		}
	} 
	else //没有已存在的会话，则创建新会话，并将其保存着在m_mapSessionHold中
	{
		session.last_time = time(NULL);
		unsigned server_num = SelectServerBasedStrategy();
		session.server_id = m_vecServerPoint[server_num].proc_id;
		m_mapSessionHold.insert(pair<unsigned,t_session_hold>(session.client_flag, session));
		TRACE("新建会话：当前所选服务器：%d  共有会话数：%d\n",server_num,m_mapSessionHold.size());

		return server_num;		 
	}
	return -1;
}

int CLoadBalancer::SessionOverTimeCheck()
{
	g_cs_SessionMag.Lock();
	map<unsigned,t_session_hold>::iterator iter = m_mapSessionHold.begin();
    //Notice：map在删除元素时，需注意迭代器变化
	for (; iter != m_mapSessionHold.end(); )
	{
		if (time(NULL)- iter->second.last_time >= m_sessionInterval) //超过30秒，则删除该会话
			m_mapSessionHold.erase(iter++);
		else
			iter++;
	}
	int session_num = m_mapSessionHold.size();
	TRACE("共有会话：%d\n",session_num);
	g_cs_SessionMag.Unlock();
	return session_num;
}
