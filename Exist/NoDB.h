// NoDB.h: interface for the NoDB class.
//
//////////////////////////////////////////////////////////////////////

#ifndef NOSQL_H
#define NOSQL_H

#include <ctime>
#include "../Micro-Development-Kit/include/frame/netserver/STNetServer.h"
#include "../Micro-Development-Kit/include/mdk/ConfigFile.h"
#include "../Micro-Development-Kit/include/mdk/Signal.h"
#include "../Micro-Development-Kit/include/mdk/Logger.h"
#include "../Micro-Development-Kit/include/mdk/Thread.h"
#include "../Micro-Development-Kit/include/mdk/Lock.h"
#include "RHTable.h"
#include <string>

#include "../bstruct/include/BArray.h"

#include "../common/common.h"
#include "../common/Protocol.h"

class NoDB : public mdk::STNetServer
{
/*
 *	NoDB������
 */
typedef struct NO_DB_CONTEXT
{
	mdk::STNetHost host;
	bool bConnected;
	std::string distributed;
	std::string guideIP;
	unsigned short guidePort;
	std::string databaseIP;
	unsigned short databasePort;
	std::string masterIP;
	unsigned short masterPort;
	
	std::string role;
	/*
		��Ƭ��
		distributed=true����role=pieceʱ�������ã��������
		piece���͵Ľ�㸺���ṩ������������ڵ�����������
		Ƭ�Ŵ�0��ʼ,Ƭ�ŵļ��㹫ʽΪ:���ݵ�hashvalue/��Ƭ��С
		������������hashvalue����1234,��Ƭ��С������Ϊ100,��ô�����ݾͱ���Ϊ��11����Ƭ
		0�ŷ�Ƭ����hashvalueΪ0~99������ 0~99֮���κ�����/100 = 0,
		1�ŷ�Ƭ����hashvalueΪ100~199������ 100~199֮���κ�����/100 = 1,
		...
		10�ŷ�Ƭ����hashvalueΪ1000~1999������ 1000~1999֮���κ�����/100 = 10,

		����С��0��Ƭ��ʱ,��ʾ��Ƭ������,����������,ֻ�ǹ��������д�����
		�ȴ��û�ָ��ȷ���Լ���Ƭ�����ݽ��м���,�ṩ����
	*/
	int pieceNo;
	std::string dataRootDir;
	unsigned short port;
	std::string wanIP;
	std::string lanIP;
	unsigned int maxMemory;
	bool m_bStop;
}NO_DB_CONTEXT;

public:
	NoDB(char* cfgFile);
	virtual ~NoDB();

public:
	mdk::Logger& GetLog();//ȡ����־����
		
protected:
	/*
	 *	���������߳�
	 */
	void* Main(void* pParam);
	
	//���ӵ�����Ӧ
	void OnConnect(mdk::STNetHost &host);
	//���ݵ�����Ӧ
	void OnMsg(mdk::STNetHost &host);
	//����Guide�����ϵ����ݲ��������host����Guide��ʲô������������false
	bool OnGuideMsg( mdk::STNetHost &host );
	//����client�����ϵ����ݲ��������host��Guide��ʲô������������false
	bool OnClientMsg( mdk::STNetHost &host );
	//���ӶϿ���Ӧ
	void OnCloseConnect(mdk::STNetHost &host);
	//���У����Ӧ
	void OnNodeCheck( mdk::STNetHost &host, bsp::BStruct &msg );
	//�������ݿ�
	void OnSetDatabase( mdk::STNetHost &host, bsp::BStruct &msg );
	//������Ƭ
	void OnSetMaster( mdk::STNetHost &host, bsp::BStruct &msg );
		
private:
	//�˳�����
	void* RemoteCall Exit(void* pParam);
	bool CreateDir( const char *strDir );
	bool CreateFile( const char *strFile );
	/*
		��Ӳ�̶�ȡ����
		����Exist��ֲ�ʽExist�����ݿ��㣬����
	*/
	void ReadData();
	/*
		��������
		�ֲ�ʽExist�ķ����ݿ��㣬����
	 */
	void* RemoteCall DownloadData(void* pParam);
	void Heartbeat( time_t tCurTime );//����
	void SaveData(time_t tCurTime);//���ݳ־û�
	void ConnectGuide(mdk::STNetHost &host);//���ӵ��򵼷���
	void ConnectMaster(mdk::STNetHost &host);//���ӵ���Ƭ
	void ConnectDB(mdk::STNetHost &host);//���ӵ����ݿ�
	void SendReadyNotify();//���;���֪ͨ
	
private:
	mdk::Logger m_log;
	mdk::ConfigFile m_cfgFile;
	NO_DB_CONTEXT m_context;//NoDB�����ģ�����������ģʽ����ɫ�ȣ�
	mdk::Thread t_exit;//�˳������߳�
	unsigned char *m_msgBuffer;//��Ϣ����
	NodeStatus::NodeStatus m_status;//״̬
	mdk::Signal m_waitCheck;//�ȴ�Guide��֤���
	mdk::Signal m_waitMain;//�ȴ����̼߳������
	bool m_bCheckVaild;//��֤���Ϊ��Ч
	bool m_bGuideConnected;//�Ƿ����ӵ�guide
	mdk::STNetHost m_guideHost;//guide����
	mdk::STNetHost m_databaseHost;//���ݿ�
	mdk::STNetHost m_masterHost;//��Ƭ
	mdk::Thread t_loaddata;//���ݼ����߳�
	mdk::Mutex m_ioLock;//io��
	bool m_bNotifyReady;//����֪ͨ�ѷ���
	RHTable m_nosqlDB;
};

#endif // !defined NOSQL_H
