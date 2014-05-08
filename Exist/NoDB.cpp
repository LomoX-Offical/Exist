// NoDB.cpp: implementation of the NoDB class.
//
//////////////////////////////////////////////////////////////////////

#include "NoDB.h"

#include <cstdio>
#include <cstdlib>
#include <io.h>
#include <direct.h>
#ifdef WIN32
#else
#include <unistd.h>
#endif

#ifdef WIN32
#else
#include <sys/time.h>
#endif

#include "../Micro-Development-Kit/include/mdk/Socket.h"

using namespace std;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

bool NoDB::CreateDir( const char *strDir )
{
	string path = strDir;
	int startPos = 0;
	int pos = path.find( '\\', startPos );
	string dir;
	while ( true )
	{
		if ( -1 == pos ) dir = path;
		else dir.assign( path, 0, pos );
		if ( -1 == access( dir.c_str(), 0 ) )
		{
#ifdef WIN32
			if( 0 != mkdir(dir.c_str()) ) return false;
#else
			umask(0);
			if( 0 != mkdir(strDir, 0777) ) return false;
			umask(0);
			chmod(strDir,0777);
#endif
		}
		if ( -1 == pos ) break;
		startPos = pos + 1;
		pos = path.find( '\\', startPos );
	}
	
	return true;
}

bool NoDB::CreateFile( const char *strFile )
{
	if ( 0 == access( strFile, 0 ) ) 
	{
#ifndef WIN32
		umask(0);
		chmod(strFile,0777);
#endif
		return true;
	}
	
	string file = strFile;
	int pos = file.find_last_of( '\\', file.size() );
	if ( -1 != pos ) 
	{
		string dir;
		dir.assign( file, 0, pos );
		if ( !CreateDir(dir.c_str()) ) return false;
	}
	FILE *fp = fopen( strFile, "w" );
	if ( NULL == fp ) return false;
	fclose( fp );
#ifdef WIN32
#else
	umask(0);
	chmod(strFile,0777);
#endif
	return true;
}

NoDB::NoDB(char* cfgFile)
:m_cfgFile(cfgFile)
{
	m_msgBuffer = new unsigned char[MAX_MSG_SIZE];
	if ( NULL == m_msgBuffer ) 
	{
		printf( "�ڴ治��\n" );
		return;
	}

	m_bGuideConnected = false;
	m_bNotifyReady = false;
	m_status = NodeStatus::Idle;
	m_context.m_bStop = false;
	m_context.distributed = (string)m_cfgFile["distributed"];
	m_context.guideIP = (string)m_cfgFile["guide ip"];
	m_context.guidePort = m_cfgFile["guide port"];
	m_context.databaseIP = (string)"";
	m_context.databasePort = 0;
	m_context.masterIP = (string)"";
	m_context.masterPort = 0;
	m_context.role = (string)m_cfgFile["role"];
	m_context.pieceNo = m_cfgFile["pieceNo"];
	m_context.dataRootDir = (string)m_cfgFile["data root dir"];
	m_context.port = m_cfgFile["port"];
	m_context.wanIP = (string)m_cfgFile["wan IP"];
	m_context.lanIP = (string)m_cfgFile["lan IP"];
	m_context.maxMemory = m_cfgFile["max memory"];

	if ( "true" == m_context.distributed ) 
	{
		Connect( m_context.guideIP.c_str(), m_context.guidePort );
		if ( "database" == m_context.role ) CreateDir( m_context.dataRootDir.c_str() );
	}
	else 
	{
		CreateDir( m_context.dataRootDir.c_str() );
		m_waitCheck.Notify();
	}
	m_nosqlDB.SetRemoteMode(true);
	m_bCheckVaild = true;
}

NoDB::~NoDB()
{
	if ( NULL != m_msgBuffer )
	{
		delete[]m_msgBuffer;
		m_msgBuffer = NULL;
	}
}

mdk::Logger& NoDB::GetLog()
{
	return m_log;
}

void* NoDB::Exit(void* pParam)
{
	m_context.m_bStop = true;
	Stop();
	return 0;
}

/*
 *	���������߳�
 *  ��ʱд�ļ�
 */
void* NoDB::Main(void* pParam)
{
	if ( "true" == m_context.distributed ) 
	{
		if ( !m_waitCheck.Wait( 3000 ) )
		{
			printf( "�޷�������\n" );
			t_exit.Run(mdk::Executor::Bind(&NoDB::Exit), this, NULL);
			return 0;
		}
	}
	else m_waitCheck.Wait();
	if ( !m_bCheckVaild ) return 0;
	if ( "true" == m_context.distributed ) 
	{
		printf( "�ֲ�ʽ��\n" );
		printf( "���������֤ͨ��\n" );
		if ( "database" == m_context.role ) printf( "��ǰ��ɫ�����ݿ�\n" );
		else if ( "master" == m_context.role ) printf( "��ǰ��ɫ����Ƭ\n" );
		else if ( "piece" == m_context.role ) printf( "��ǰ��ɫ����Ƭ\n", m_context.pieceNo );
	}
	else 
	{
		printf( "�ֲ�ʽδ��\n" );
	}
	m_waitMain.Notify();
	ReadData();//��Ӳ�̶�����
	//��̨ѭ��ҵ��
	time_t tCurTime = 0;
	while ( !m_context.m_bStop )
	{
		tCurTime = time( NULL );
		Heartbeat( tCurTime );//��������
		SaveData( tCurTime );//��������
		Sleep( 1000 );
	}
	return 0;
}

//���ӵ�����Ӧ
void NoDB::OnConnect(mdk::STNetHost &host)
{
	string ip;
	int port;
	host.GetServerAddress( ip, port );
	//���ӵ�guide
	if ( ip == m_context.guideIP && port == m_context.guidePort )
	{
		ConnectGuide(host);
		return;
	}
	//���ӵ����ݿ�
	if ( ip == m_context.databaseIP && port == m_context.databasePort )
	{
		ConnectDB(host);
		return;
	}
	//���ӵ���Ƭ
	if ( ip == m_context.masterIP && port == m_context.masterPort )
	{
		ConnectMaster(host);
		return;
	}
	//client���ӽ���
}

//���ӶϿ���Ӧ
void NoDB::OnCloseConnect(mdk::STNetHost &host)
{
	if ( host.ID() == m_guideHost.ID() ) 
	{
		printf( "Guide�Ͽ�\n" );
		m_bGuideConnected = false;
		m_bNotifyReady = false;
		return;
	}
	if ( host.ID() == m_databaseHost.ID() ) 
	{
		printf( "���ݿ�Ͽ�\n" );
		return;
	}
	if ( host.ID() == m_masterHost.ID() ) 
	{
		printf( "��Ƭ�Ͽ�\n" );
		return;
	}
	printf( "client�Ͽ�\n" );
}

//���ݵ�����Ӧ
void NoDB::OnMsg(mdk::STNetHost &host)
{
	if ( OnGuideMsg( host ) ) return;
	OnClientMsg( host );
}

//����Guide�����ϵ����ݲ��������host����Guide��ʲô������������false
bool NoDB::OnGuideMsg( mdk::STNetHost &host )
{
	if ( host.ID() != m_guideHost.ID() ) return false;
	//////////////////////////////////////////////////////////////////////////
	//����BStructЭ���ʽ�ı���
	bsp::BStruct msg;
	if ( !host.Recv(m_msgBuffer, MSG_HEAD_SIZE, false) ) return true;
	unsigned int len = bsp::memtoi(&m_msgBuffer[1], sizeof(unsigned int));
	if ( len > MAX_BSTRUCT_SIZE ) 
	{
		host.Recv( m_msgBuffer, MSG_HEAD_SIZE );
		return true;
	}
	if ( !host.Recv(m_msgBuffer, len + MSG_HEAD_SIZE) ) return true;
	DataFormat::DataFormat df = (DataFormat::DataFormat)m_msgBuffer[0];
	if ( DataFormat::BStruct != df ) return true;//ֻ����BStruct��ʽ�ı���
	if ( !msg.Resolve(&m_msgBuffer[MSG_HEAD_SIZE],len) ) return true;//��������
	
	//////////////////////////////////////////////////////////////////////////
	//������
	if ( !msg["msgid"].IsValid() || sizeof(unsigned short) != msg["msgid"].m_size ) return true;
	
	unsigned short msgid = msg["msgid"];
	bool bIsInvalidMsg = true;//Ĭ��Ϊ�Ƿ�����
	switch( msgid )
	{
	case msgid::gNodeCheck :
		OnNodeCheck( host, msg );
		break;
	case msgid::gSetDatabase :
		OnSetDatabase( host, msg );
		break;
	case msgid::gSetMaster :
		OnSetMaster( host, msg );
		break;
	default:
		break;
	}
	return true;
}

/*
 *	2 byte ���ĳ���
 *	2 byte key����
 *	n byte key
 *	2 byte value����
 *	n byte value
 *	4 byte hashvalue
 */
unsigned int g_insertCount = 0;
#ifdef WIN32
SYSTEMTIME t1,t2;
#else
struct timeval tEnd,tStart;
struct timezone tz;
#endif
//����client�����ϵ����ݲ��������host��Guide��ʲô������������false
bool NoDB::OnClientMsg( mdk::STNetHost &host )
{
	if ( host.ID() == m_guideHost.ID() ) return false;
	unsigned char msg[600];
	unsigned short len = 2;
	
	if ( !host.Recv( msg, len, false ) ) 
	{
		return true;
	}
	memcpy( &len, msg, sizeof(unsigned short) );
	if ( 2 >= len ) //�Ƿ�����
	{
		printf( "del msg\n" );
		host.Recv( msg, len );//ɾ������
		return true;
	}
	len += 2;
	if ( !host.Recv( msg, len ) ) 
	{
		return true;
	}
	unsigned short keySize = 0;
	unsigned short valueSize = 0;
	memcpy( &keySize, &msg[2], sizeof(unsigned short) );
	if ( 2 + 2 + keySize > len ) 
	{
		printf( "del msg 2 + 2 + keySize > len\n" );
		return true;//�Ƿ�����
	}
	memcpy( &valueSize, &msg[2+2+keySize], sizeof(unsigned short) );
	if ( 2 + 2 + keySize + 2 + valueSize + 4 > len ) 
	{
		printf( "del msg 2 + 2 + keySize + 2 + valueSize + 4 > len\n" );
		return true;//�Ƿ�����
	}
	unsigned char *key = &msg[2+2];
	unsigned char *value = NULL;//new unsigned char[valueSize];
	//	memcpy( value, &msg[2+2+keySize+2], valueSize );
	value = &msg[2+2+keySize+2];
	unsigned int hashValue = 0;
	memcpy( &hashValue, &msg[2+2+keySize+2+valueSize], sizeof(unsigned int) );
	RHTable::OP_R *pR = m_nosqlDB.Insert(key, keySize, value, hashValue );
	g_insertCount++;
	if ( 1 == g_insertCount )
	{
#ifdef WIN32
		GetSystemTime(&t1);
#else
		gettimeofday (&tStart , &tz);
#endif
	}
	if ( g_insertCount >= 200000 )
	{
		g_insertCount = 0;
#ifdef WIN32
		GetSystemTime(&t2);
		int s = ((t2.wHour - t1.wHour)*3600 + (t2.wMinute - t1.wMinute) * 60 
			+ t2.wSecond - t1.wSecond) * 1000 + t2.wMilliseconds - t1.wMilliseconds;
		printf( "%f\n", s / 1000.0 );
#else
		gettimeofday (&tEnd , &tz);
		printf("time:%ld\n", (tEnd.tv_sec-tStart.tv_sec)*1000+(tEnd.tv_usec-tStart.tv_usec)/1000);
#endif
	}

	return true;
}

/*
	��Ӳ�̶�ȡ����
	����Exist��ֲ�ʽExist�����ݿ��㣬����
*/
void NoDB::ReadData()
{
	if ( "true" == m_context.distributed && "database" != m_context.role ) return;
	printf( "��ʼ��������\n" );
	Listen(m_context.port);//��ʼ�ṩ����
	m_status = NodeStatus::Serving;
	SendReadyNotify();//���;���֪ͨ
	printf( "�������\n" );
	printf( "��ʼ����\n" );
	return;
}

/*
	��������
	�ֲ�ʽExist�ķ����ݿ��㣬����
*/
void* NoDB::DownloadData(void* pParam)
{
	if ( "false" == m_context.distributed || "database" == m_context.role ) return 0;
	printf( "��ʼ��������\n" );
	m_status = NodeStatus::LoadData;
	if ( "master" == m_context.role ) 
	{
		printf( "��ʼ����\n" );
		Listen(m_context.port);//��ʼ�ṩ����
		m_status = NodeStatus::Serving;
		SendReadyNotify();//���;���֪ͨ
	}

	return 0;
}

//���ݳ־û�
void NoDB::SaveData( time_t tCurTime )
{
	static time_t tLastSave = tCurTime;
	if ( tCurTime - tLastSave <= 60 ) return;
	tLastSave = tCurTime;
}

//����
void NoDB::Heartbeat( time_t tCurTime )
{
	static time_t tLastHeart = tCurTime;
	if ( !m_bGuideConnected ) return; //δ�������ӣ�����������
	if ( tCurTime - tLastHeart <= 60 ) return; //���ͼ��δ��1���ӣ�������
	bsp::BStruct msg;
	mdk::AutoLock lock(&m_ioLock);
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eHeartbeat;
	SendBStruct(m_guideHost, msg);
	tLastHeart = tCurTime;
}

//���ӵ��򵼷���
void NoDB::ConnectGuide(mdk::STNetHost &host)
{
	printf( "�ɹ����ӵ���\n" );
	m_guideHost = host;
	//���ͽ�����֪ͨ
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eNodeJoin;
	if ( "database" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Database;
	else if ( "master" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Master;
	else if ( "piece" == m_context.role ) msg["role"] = (unsigned char) NodeRole::Piece;
	msg["wan_ip"] = m_context.wanIP.c_str();
	msg["lan_ip"] = m_context.lanIP.c_str();
	msg["port"] = (unsigned short)m_context.port;
	msg["pieceNo"] = (unsigned int)m_context.pieceNo;
	SendBStruct(m_guideHost, msg);
	m_bGuideConnected = true;
	if ( "database" == m_context.role ) SendReadyNotify();//���;���֪ͨ
}

//���У����Ӧ
void NoDB::OnNodeCheck( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char role = msg["role"];
	string reason;
	if ( NodeRole::Unknow == role ) //����ɫ�Ƿ�������ԭ��
	{
		reason = (string)msg["reason"];
		host.Close();
		printf( "�����֤ʧ�ܣ�%s\n", reason.c_str() );
		t_exit.Run(mdk::Executor::Bind(&NoDB::Exit), this, NULL);
		m_bCheckVaild = false;
		m_waitCheck.Notify();
		return;
	}
	if ( NodeRole::Master == role ) m_context.role = "master";
	m_context.pieceNo = msg["pieceNo"];	//����ɫΪ��Ƭ������Ƭ����Ҫ����Ƭ��
	m_bCheckVaild = true;
	m_waitCheck.Notify();
	m_waitMain.Wait();//�ȴ����̼߳���
}

//�������ݿ�
void NoDB::OnSetDatabase( mdk::STNetHost &host, bsp::BStruct &msg )
{
	string wanIP = msg["wan_ip"];
	string lanIP = msg["lan_ip"];
	short port = msg["port"];
	printf( "�������ݿ�������·������%s,����%s,�˿�%d\n",
		lanIP.c_str(), wanIP.c_str(), port );	
	//��������������
	mdk::Socket ipcheck;
	if ( !ipcheck.Init( mdk::Socket::tcp ) ) 
	{
		printf( "Error: NoDB::OnSetDatabase �޷��������Զ���\n" );
		return;
	}
	if ( ipcheck.Connect( lanIP.c_str(), port) ) 
	{
		ipcheck.Close();
		printf( "��������·���������ݿ�\n" );	
		m_context.databaseIP = lanIP;
		m_context.databasePort = port;
		Connect( m_context.databaseIP.c_str(), m_context.databasePort );
		return;
	}
	//����������ͨ��
	ipcheck.Close();
	if ( !ipcheck.Init( mdk::Socket::tcp ) ) 
	{
		printf( "Error: NoDB::OnSetDatabase �޷��������Զ���\n" );
		return;
	}
	if ( !ipcheck.Connect(wanIP.c_str(), port) ) 
	{
		ipcheck.Close();
		printf( "�޷��������ݿ⣺����IP%s,����IP%s,�˿�%d\n", 
			lanIP.c_str(), wanIP.c_str(), port );
		return;
	}
	ipcheck.Close();
	printf( "��������·���������ݿ�\n" );	
	m_context.databaseIP = wanIP;
	m_context.databasePort = port;
	Connect( m_context.databaseIP.c_str(), m_context.databasePort );
}

//������Ƭ
void NoDB::OnSetMaster( mdk::STNetHost &host, bsp::BStruct &msg )
{
	string wanIP = msg["wan_ip"];
	string lanIP = msg["lan_ip"];
	short port = msg["port"];
}

//���ӵ����ݿ�
void NoDB::ConnectDB(mdk::STNetHost &host)
{
	printf( "�ɹ����ӵ����ݿ�\n" );
	m_databaseHost = host;
	t_loaddata.Run( mdk::Executor::Bind(&NoDB::DownloadData), this, 0 );//�����ݿ�/��Ƭ�϶�
}

//���ӵ���Ƭ
void NoDB::ConnectMaster(mdk::STNetHost &host)
{
	printf( "�ɹ����ӵ���Ƭ\n" );
	m_masterHost = host;
}

//���;���֪ͨ
void NoDB::SendReadyNotify()
{
	if ( NodeStatus::Serving != m_status ) return;
	mdk::AutoLock lock( &m_ioLock );
	if ( -1 == m_guideHost.ID() ) return;
	if ( m_bNotifyReady ) return;
	bsp::BStruct msg;
	m_bNotifyReady = true;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::eNodeReady;
	SendBStruct(m_guideHost, msg);
}

