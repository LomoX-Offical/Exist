// TCPWorker.h: interface for the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#ifndef TCPWORKER_H
#define TCPWORKER_H

#include "../Micro-Development-Kit/include/frame/netserver/STNetServer.h"
#include "../Micro-Development-Kit/include/mdk/ConfigFile.h"
#include "../Micro-Development-Kit/include/mdk/Logger.h"
#include "../bstruct/include/BStruct.h"
#include "../common/common.h"

#include <vector>
#include <map>
using namespace std;

#define MAX_BSTRUCT_SIZE	1048576 //BStruct�ṹ��󳤶�1M
#define MAX_MSG_SIZE		MAX_BSTRUCT_SIZE + MSG_HEAD_SIZE//��Ϣ������󳤶ȣ�����ͷ+BStruct����С

/*
	tcp������
	����tcp��Ϣ��ִ��ҵ��
 */
class TCPWorker : public mdk::STNetServer
{
	typedef struct Exist
	{
		mdk::STNetHost host;//����
		std::string wanIP;//����ip
		std::string lanIP;//����ip
		unsigned short port;//����˿�
		NodeRole::NodeRole role;//��ɫ
		NodeStatus::NodeStatus status;//״̬
		unsigned int pieceNo;//Ƭ��0��ʼ = hashid/Ƭ��С���ý�㸺�𱣴��hashid��Ƭ�ϵ����� 
	}Exist;
	/*
	 *	���ӳ���
	 *	key = host.ID()
	 */
	typedef map<int, Exist*> NodeList;
	/*
	 *	��Ƭӳ���
	 *	key = ��Ƭid
	 */
	typedef map<int, Exist*> PieceList;
public:
	TCPWorker( char *cfgFile );
	virtual ~TCPWorker();
	mdk::Logger& GetLog();

private:
	bool ReadConfig();
	void InvalidMsg(mdk::STNetHost &host, unsigned char *msg, unsigned short len);//�Ƿ�����
	void OnMsg(mdk::STNetHost &host);
	void OnCloseConnect(mdk::STNetHost &host);
	//////////////////////////////////////////////////////////////////////////
	//������Ӧ
	bool OnNodeJoin(mdk::STNetHost &host, bsp::BStruct &msg);//������ֲ�����
	bool OnNodeReady(mdk::STNetHost &host);//������
	bool OnNodeExit(mdk::STNetHost &host);//����˳�	
	bool OnUserCommand(mdk::STNetHost &host, bsp::BStruct &msg);//��Ӧ�˹�����ָ��
	bool OnQueryNodeList(mdk::STNetHost &host);//��ѯ���н��
	bool OnStartPiece(mdk::STNetHost &host, bsp::BStruct &msg);//��ʼ��Ƭָ��
	bool OnNodeDel(mdk::STNetHost &host, bsp::BStruct &msg);//ɾ�����ָ��
	
private:
	//////////////////////////////////////////////////////////////////////////
	//ҵ����Worker.cpp�ж���
	void DBJoin(mdk::STNetHost &host, bsp::BStruct &msg);//���ݿ���뵽������
	void MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg);//����Ƭ���뵽������
	void PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg);//��Ƭ���뵽������
	void DatabaseReady();//���ݿ����
	void DBExit();//���ݿ��˳�
	char* StartPiece(unsigned int pieceNo);//��ʼ��Ƭ���ɹ�����NULL,ʧ�ܷ���ԭ��
	char* DelNode(Exist *pExist);//ɾ��һ����㣬�ɹ�����NULL,ʧ�ܷ���ԭ��
	int NewPieceNo();//����һ����Ƭ��
		
	//////////////////////////////////////////////////////////////////////////
	//�ظ���ϢReplyMsg.cpp�ж���
	void NodeCheck(mdk::STNetHost &host, unsigned char role, int pieceNo, char *reason );//�����ϢУ��
	void SetDatabase(mdk::STNetHost &host);//�������ݿ�
	void SetMaster(mdk::STNetHost &host);//��������Ƭ
	void NodeInit(Exist *pExist);//֪ͨexist����ʼ����Ƭ
	void NodeStop(Exist *pExist);//֪ͨexist���ֹͣ����
	void NodeExit(Exist *pExist);//֪ͨexist����˳�����

	/*
		�û�����ָ̨���������ظ�
		reason=NULL��ʾ�����ɹ�
		msg�ûظ���Ӧ�Ĳ���ָ��
	*/
	void ReplyUserResult(mdk::STNetHost &host, bsp::BStruct &msg, char *reason);
	void SendSystemStatus(mdk::STNetHost &host);//����ϵͳ״̬�����н����Ϣ��
	

private:
	mdk::Logger m_log;//����log
	mdk::ConfigFile m_cfgFile;//�����ļ�
	Exist m_db;//�־û����ݿ�
	Exist m_master;//����Ƭ
	NodeList m_nodes;//���߽���б�
	PieceList m_pieces;//��Ƭ�б�
	int m_count;//���exist��
	int m_pieceSize;//Ƭ��С
	int m_port;//����exist�������ӵĶ˿�
	int m_heartTime;//exist����ʱ��
	unsigned char *m_msgBuffer;//��Ϣ����

};

#endif // !defined TCPWORKER_H
