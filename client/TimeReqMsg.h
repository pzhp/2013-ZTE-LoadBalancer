
#ifndef  TimeReqMsg
# define TimeReqMsg

#define CMDTYPE_TIMEREQ   0x00    //时间请求
#define CMDTYPE_TIMEACK   0x01    //时间应答
#define CMDTYPE_HEARTREQ  0x02    //心跳请求
#define CMDTYPE_HEARTACK  0x03    //心跳应答
  
typedef struct
{
    unsigned src_id;    //消息的发送进程是谁，就填谁的id
    unsigned dst_id;    //消息的接收进程是谁，就填谁的id
    unsigned usr_id;    //发送"时间请求"消息时填写，	回复"时间应答"消息时，其值要与请求消息保持一致
    unsigned msg_type;  //消息类型：0, 时间请求；1, 时间答应；2, 心跳请求；3, 心跳应答
    char data[32];      //服务端回复"时间应答"消息时，在data中填入当前时间的字符串，形式如"2013-06-20 13:56:28"即可 
} t_msg;

typedef struct  
{
	unsigned int recv_num;        //接收消息总数
	unsigned int send_num;        //发送消息总数
	unsigned int correct_inrecv;  //接收中正确数
	unsigned int wrong_inrecv;    //接收中错误数
	
} t_msg_statistics;

#endif