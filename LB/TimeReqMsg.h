
#ifndef  TimeReqMsg
# define TimeReqMsg

#define CMDTYPE_TIMEREQ   0x00    //ʱ������
#define CMDTYPE_TIMEACK   0x01    //ʱ��Ӧ��
#define CMDTYPE_HEARTREQ  0x02    //��������
#define CMDTYPE_HEARTACK  0x03    //����Ӧ��
  
typedef struct
{
    unsigned src_id;    //��Ϣ�ķ��ͽ�����˭������˭��id
    unsigned dst_id;    //��Ϣ�Ľ��ս�����˭������˭��id
    unsigned usr_id;    //����"ʱ������"��Ϣʱ��д��	�ظ�"ʱ��Ӧ��"��Ϣʱ����ֵҪ��������Ϣ����һ��
    unsigned msg_type;  //��Ϣ���ͣ�0, ʱ������1, ʱ���Ӧ��2, ��������3, ����Ӧ��
    char data[32];      //����˻ظ�"ʱ��Ӧ��"��Ϣʱ����data�����뵱ǰʱ����ַ�������ʽ��"2013-06-20 13:56:28"���� 
} t_msg;

typedef struct  
{
	unsigned int recv_num;        //������Ϣ����
	unsigned int send_num;        //������Ϣ����
	unsigned int correct_inrecv;  //��������ȷ��
	unsigned int wrong_inrecv;    //�����д�����
	
} t_msg_statistics;

#endif