// TimeServer.h: interface for the CTimeServer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_TIMESERVER_H__599D972E_7BB2_4347_8AAD_C98B0FD02ED2__INCLUDED_)
#define AFX_TIMESERVER_H__599D972E_7BB2_4347_8AAD_C98B0FD02ED2__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "TimeReqMsg.h"
#include <PROCESS.H>
#include <CTIME>

class CTimeServer  
{
public:	

	CTimeServer();
	virtual ~CTimeServer();
	
	bool StartTimeServer(unsigned id, short port);    // 开启服务，传入线程id，UDP端口号
	UINT GetServerProcID();                           //返回服务线程ID号
	void ClearUpTimeServer();                         //结束服务
	
private:
	void AddMsgReport(t_msg msg_tmp);                 // 通过消息模型将数据发给UI主线程，在那里进行消息显示和统计
	bool MsgReqAnswer(t_msg recmsg, t_msg& senmsg);   // 由接收到的数据生成响应指令(进程号不匹配则丢弃)，第二个参数需传入引用型
	static bool recv_thread(CTimeServer* pServer);    // 接收请求，并返回运算结果
	
private:
	bool       m_thread_run;     //收发线程标志量，0表示线程退出
	SOCKET     m_server_socket;  // 服务器套接字
	unsigned   m_server_id;      //服务器用户制定的进程ID
	short      m_udp_port;       //绑定的端口
	HWND       m_ui_hwnd;        //初始化为主窗口的句柄，发送消息时用
};

#endif // !defined(AFX_TIMESERVER_H__599D972E_7BB2_4347_8AAD_C98B0FD02ED2__INCLUDED_)
