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
	
	bool StartTimeServer(unsigned id, short port);    // �������񣬴����߳�id��UDP�˿ں�
	UINT GetServerProcID();                           //���ط����߳�ID��
	void ClearUpTimeServer();                         //��������
	
private:
	void AddMsgReport(t_msg msg_tmp);                 // ͨ����Ϣģ�ͽ����ݷ���UI���̣߳������������Ϣ��ʾ��ͳ��
	bool MsgReqAnswer(t_msg recmsg, t_msg& senmsg);   // �ɽ��յ�������������Ӧָ��(���̺Ų�ƥ������)���ڶ��������贫��������
	static bool recv_thread(CTimeServer* pServer);    // �������󣬲�����������
	
private:
	bool       m_thread_run;     //�շ��̱߳�־����0��ʾ�߳��˳�
	SOCKET     m_server_socket;  // �������׽���
	unsigned   m_server_id;      //�������û��ƶ��Ľ���ID
	short      m_udp_port;       //�󶨵Ķ˿�
	HWND       m_ui_hwnd;        //��ʼ��Ϊ�����ڵľ����������Ϣʱ��
};

#endif // !defined(AFX_TIMESERVER_H__599D972E_7BB2_4347_8AAD_C98B0FD02ED2__INCLUDED_)
