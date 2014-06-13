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

#define  CLIENT_STATISTICS    0x00     //客户端统计标志
#define  SERVER_STATISTICS    0x01     //服务端统计标志

//单个服务器信息，需从配置文件读取
struct t_server_info
{
	UINT         proc_id;             //服务器的进程ID
	SOCKADDR_IN  server_addr;         //地址信息（AF_INET IP，UDP端口）
	unsigned     respond_speed;       //服务器心跳包的响应速度,相对单位： 最快响应分发策略
	char         ratio;               //服务器权重值，取值范围1-10：权重分发策略用	
	char         unrecv_heart_count;  //心跳计数，连续4次收不到，则判定服务器故障
};

//会话保持功能
struct t_session_hold
{
	unsigned client_flag;             //客户端标志,usr_id | src_id
	unsigned server_id;               //服务端id
	long     last_time;               //最后一次时间请求的时间戳
};

class CLoadBalancer  
{
public:
	enum em_DistributeStrategy 
	{Poll_Dist, Ratio_Dist, Fast_Dist};        //LB分发策略：{轮转方式，权重方式，最快响应速度方式}
	
	enum em_SessionHold 
	{No_Session, Based_Src_id, Based_Usr_id};  //会话保持功能：{无会话保持，基于src_id方式，基于usr_id方式}

	CLoadBalancer();
	virtual ~CLoadBalancer();
	bool StartLBServer();            //开启服务
	int  GetOnlineServerNum();       //返回在线服务端的数量
	
private:	
	bool ReadConfigInfo();                          //从配置文件读取相关信息
	bool SetSockConfig();                           //服务端、客户端套接字绑定
	void AddMsgReport(t_msg msg_tmp, int direct);   //统计信息，并发给界面线程显示，线程安全(消息，客户端或服务端)
	
	int  PollDistributeStrategy();                  //轮转分发策略, 返回服务器编号
	int  RatioDistributeStrategy();                 //权重分发策略, 返回服务器编号
	int  FastRespondDistStrategy();                 //最快响应分发策略, 返回服务器编号
	int  SelectServerBasedStrategy();               //根据配置文件，采用特定算法选择服务器分发时间请求, 返回服务器编号
	int  SelectServerOnHoldSession(t_msg recv_temp);//在会话保持的功能上，通过 SelectServerBasedStrategy() 选取服务器, 返回服务器编号	
	int  SessionOverTimeCheck();                    //实现会话检测功能，超过一定时间没有后续的请求，则删除该会话，返回当前会话数
	

	void WriteLog(CString description ,CString reason, CString other);   //写日志，线程安全 (描述，原因，其他因素)
	
	static void ack_out_thread(CLoadBalancer *pLB);	     //从内部服务器接收反馈，并转发给外部客户端
	static void req_in_thread(CLoadBalancer* pLB);       //从外面客户端接受请求，并转发给内部服务器
	static void heart_check_thread(CLoadBalancer* pLB);  //心跳检查，检查服务器是否在线
	
public:
	t_msg_statistics m_client_stat;  //客户端套接字收发数据统计
	t_msg_statistics m_server_stat;  //服务器套接字收发数据统计
	UINT m_localProcID;              //当前LB的进程ID
	UINT m_client_udp_port;          // 客户端套接字绑定的UDP端口
	UINT m_server_udp_port;	         // 服务端套接字绑定的UDP端口
	multimap<unsigned, SOCKADDR_IN> m_mapClientAddr;     //缓存客户端地址信息，键为usr_id，值为客户端地址
	map<unsigned, t_session_hold>   m_mapSessionHold;    //会话保持功能，键为客服端src_id或usr_id，根据配置文件设定
	
private:

	CString m_strConfigFileName;                       //配置文件名
	CString m_strLogFileName;                          //日志文件名
	vector<t_server_info> m_vecServerPoint;            //管理在线服务器信息(进程ID，地址信息)
	
	SOCKET m_server_sock;           // 服务端套接字
	SOCKET m_client_sock;           // 客户端套接字
	
	bool   m_ackout_run;            //反馈信息线程标志量，1表示继续运行，0表示终止线程
	bool   m_reqin_run;             //接收请求线程标志量，1表示继续运行，0表示终止线程
	bool   m_heart_run;             //心跳线程标志量，    1表示继续运行，0表示终止线程
	
	int    m_nextselect_server; 	                    //下次所选的服务器编号
	int    m_heartInterval;                             //心跳包发送时间间隔（ms）
	int    m_sessionInterval;                           //会话保持最长时间，持续30s没有收到该会话的客户端，则删除之
	em_DistributeStrategy   m_distributePolicy;         //分发策略：轮转，权重分发，最快响应
	em_SessionHold          m_sessionHold;              //会话保持策略
	
	HWND   m_ui_hwnd;                                   //界面主线程窗口句柄，发送消息时用
	ofstream m_ofstream;                                //日志文件的输出流
};

#endif // !defined(AFX_LOADBALANCER_H__4699CD20_42DC_4F34_95F9_609A32968374__INCLUDED_)
