// LoadBalancer.h: interface for the LoadBalancer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOADBALANCER_H__4699CD20_42DC_4F34_95F9_609A32968374__INCLUDED_)
#define AFX_LOADBALANCER_H__4699CD20_42DC_4F34_95F9_609A32968374__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include  "TimeReqMsg.h"
#include  <ctime>
#include  <vector>
#include  <PROCESS.H>
#include  <fstream>
#include  <map>
using namespace std;

#define  CLIENT_STATISTICS    0x00     //�ͻ���ͳ�Ʊ�־
#define  SERVER_STATISTICS    0x01     //�����ͳ�Ʊ�־

//������������Ϣ����������ļ���ȡ
struct t_server_info
{
	UINT         proc_id;             //�������Ľ���ID
	SOCKADDR_IN  server_addr;         //��ַ��Ϣ��AF_INET IP��UDP�˿ڣ�
	unsigned     respond_speed;       //����������������Ӧ�ٶ�,��Ե�λ�� �����Ӧ�ַ�����
	char         ratio;               //������Ȩ��ֵ��ȡֵ��Χ1-10��Ȩ�طַ�������	
	char         unrecv_heart_count;  //��������������4���ղ��������ж�����������
};

//�Ự���ֹ���
struct t_session_hold
{
	unsigned client_flag;             //�ͻ��˱�־,usr_id | src_id
	unsigned server_id;               //�����id
	long     last_time;               //���һ��ʱ�������ʱ���
};

class CLoadBalancer  
{
public:
	enum em_DistributeStrategy 
	{Poll_Dist, Ratio_Dist, Fast_Dist};        //LB�ַ����ԣ�{��ת��ʽ��Ȩ�ط�ʽ�������Ӧ�ٶȷ�ʽ}
	
	enum em_SessionHold 
	{No_Session, Based_Src_id, Based_Usr_id};  //�Ự���ֹ��ܣ�{�޻Ự���֣�����src_id��ʽ������usr_id��ʽ}

	CLoadBalancer();
	virtual ~CLoadBalancer();
	bool StartLBServer();            //��������
	int  GetOnlineServerNum();       //�������߷���˵�����
	
private:	
	bool ReadConfigInfo();                          //�������ļ���ȡ�����Ϣ
	bool SetSockConfig();                           //����ˡ��ͻ����׽��ְ�
	void AddMsgReport(t_msg msg_tmp, int direct);   //ͳ����Ϣ�������������߳���ʾ���̰߳�ȫ(��Ϣ���ͻ��˻�����)
	
	int  PollDistributeStrategy();                  //��ת�ַ�����, ���ط��������
	int  RatioDistributeStrategy();                 //Ȩ�طַ�����, ���ط��������
	int  FastRespondDistStrategy();                 //�����Ӧ�ַ�����, ���ط��������
	int  SelectServerBasedStrategy();               //���������ļ��������ض��㷨ѡ��������ַ�ʱ������, ���ط��������
	int  SelectServerOnHoldSession(t_msg recv_temp);//�ڻỰ���ֵĹ����ϣ�ͨ�� SelectServerBasedStrategy() ѡȡ������, ���ط��������	
	int  SessionOverTimeCheck();                    //ʵ�ֻỰ��⹦�ܣ�����һ��ʱ��û�к�����������ɾ���ûỰ�����ص�ǰ�Ự��
	

	void WriteLog(CString description ,CString reason, CString other);   //д��־���̰߳�ȫ (������ԭ����������)
	
	static void ack_out_thread(CLoadBalancer *pLB);	     //���ڲ����������շ�������ת�����ⲿ�ͻ���
	static void req_in_thread(CLoadBalancer* pLB);       //������ͻ��˽������󣬲�ת�����ڲ�������
	static void heart_check_thread(CLoadBalancer* pLB);  //������飬���������Ƿ�����
	
public:
	t_msg_statistics m_client_stat;  //�ͻ����׽����շ�����ͳ��
	t_msg_statistics m_server_stat;  //�������׽����շ�����ͳ��
	UINT m_localProcID;              //��ǰLB�Ľ���ID
	UINT m_client_udp_port;          // �ͻ����׽��ְ󶨵�UDP�˿�
	UINT m_server_udp_port;	         // ������׽��ְ󶨵�UDP�˿�
	multimap<unsigned, SOCKADDR_IN> m_mapClientAddr;     //����ͻ��˵�ַ��Ϣ����Ϊusr_id��ֵΪ�ͻ��˵�ַ
	map<unsigned, t_session_hold>   m_mapSessionHold;    //�Ự���ֹ��ܣ���Ϊ�ͷ���src_id��usr_id�����������ļ��趨
	
private:

	CString m_strConfigFileName;                       //�����ļ���
	CString m_strLogFileName;                          //��־�ļ���
	vector<t_server_info> m_vecServerPoint;            //�������߷�������Ϣ(����ID����ַ��Ϣ)
	
	SOCKET m_server_sock;           // ������׽���
	SOCKET m_client_sock;           // �ͻ����׽���
	
	bool   m_ackout_run;            //������Ϣ�̱߳�־����1��ʾ�������У�0��ʾ��ֹ�߳�
	bool   m_reqin_run;             //���������̱߳�־����1��ʾ�������У�0��ʾ��ֹ�߳�
	bool   m_heart_run;             //�����̱߳�־����    1��ʾ�������У�0��ʾ��ֹ�߳�
	
	int    m_nextselect_server; 	                    //�´���ѡ�ķ��������
	int    m_heartInterval;                             //����������ʱ������ms��
	int    m_sessionInterval;                           //�Ự�����ʱ�䣬����30sû���յ��ûỰ�Ŀͻ��ˣ���ɾ��֮
	em_DistributeStrategy   m_distributePolicy;         //�ַ����ԣ���ת��Ȩ�طַ��������Ӧ
	em_SessionHold          m_sessionHold;              //�Ự���ֲ���
	
	HWND   m_ui_hwnd;                                   //�������̴߳��ھ����������Ϣʱ��
	ofstream m_ofstream;                                //��־�ļ��������
};

#endif // !defined(AFX_LOADBALANCER_H__4699CD20_42DC_4F34_95F9_609A32968374__INCLUDED_)
