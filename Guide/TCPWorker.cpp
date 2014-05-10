// TCPWorker.cpp: implementation of the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

TCPWorker::TCPWorker(char *cfgFile)
:m_cfgFile( cfgFile )
{
	m_msgBuffer = new unsigned char[MAX_MSG_SIZE];
	m_count = 0;
	if ( NULL == m_msgBuffer ) return;
	m_db.role = NodeRole::Database;
	m_db.status = NodeStatus::NotOnline;
	m_master.role = NodeRole::Master;
	m_master.status = NodeStatus::NotOnline;
	ReadConfig();
	SetAverageConnectCount(1000);
	SetHeartTime( m_heartTime );
	Listen(m_port);
}

TCPWorker::~TCPWorker()
{
	if ( NULL != m_msgBuffer )
	{
		delete[]m_msgBuffer;
		m_msgBuffer = NULL;
	}
}

bool TCPWorker::ReadConfig()
{
	m_port = m_cfgFile[SECTION_KEY]["port"];
	m_heartTime = m_cfgFile[SECTION_KEY]["heartTime"];
	m_pieceSize = m_cfgFile[SECTION_KEY]["pieceSize"];
	
	if (  120 > m_heartTime ) m_heartTime = 120;
	if ( 0 >= m_port ) m_port = 7250;
	if ( 0 >= m_pieceSize ) m_pieceSize = 5000000;
	return true;
}

mdk::Logger& TCPWorker::GetLog()
{
	return m_log;
}

void TCPWorker::OnCloseConnect(mdk::STNetHost &host)
{
	if ( m_db.host.ID() == host.ID() )//���ݿ����
	{
		DBExit();
		return;
	}
	else if ( m_master.host.ID() == host.ID() )//����Ƭ����
	{
		printf( "��Ƭ�Ͽ�\n" );
		m_master.status = NodeStatus::NotOnline;
		return;
	}
	
	//������
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.find( host.ID() );
	if ( it != m_nodes.end() ) 
	{
		pExist = it->second;
		m_nodes.erase(it);
		//�����߽���Ƿ���һ����Ƭ
		PieceList::iterator itPiece = m_pieces.find( pExist->pieceNo );
		if ( itPiece != m_pieces.end() && pExist == itPiece->second ) 
		{
			printf( "��Ƭ%d�Ͽ�\n", pExist->pieceNo );
			pExist->status = NodeStatus::NotOnline;
			return;
		}
		//���߽����һ�����н��ֱ���ͷ�
		printf( "���з�Ƭ�Ͽ�\n" );
		delete pExist;
	}

	//�˹�����̨���ӶϿ�
	return;
}

//�Ƿ�����
void TCPWorker::InvalidMsg(mdk::STNetHost &host, unsigned char *msg, unsigned short len)
{
	m_log.Info("info:", "Waring: msg format is invalid from Host(%d)", host.ID() );
	m_log.Info("info:", "%s-%d", msg, len);
	host.Close();
}

/**
 * ���ݵ���ص�����
 * 
 * ������ʵ�־���Ͽ�����ҵ����
 * 
*/
void TCPWorker::OnMsg(mdk::STNetHost &host)
{
	//////////////////////////////////////////////////////////////////////////
	//����BStructЭ���ʽ�ı���
	bsp::BStruct msg;
	if ( !host.Recv(m_msgBuffer, MSG_HEAD_SIZE, false) ) return;
	unsigned int len = bsp::memtoi(&m_msgBuffer[1], sizeof(unsigned int));
	if ( len > MAX_BSTRUCT_SIZE ) 
	{
		host.Recv( m_msgBuffer, MSG_HEAD_SIZE );
		InvalidMsg(host, m_msgBuffer, MSG_HEAD_SIZE);
		return;
	}
	if ( !host.Recv(m_msgBuffer, len + MSG_HEAD_SIZE) ) return;
	DataFormat::DataFormat df = (DataFormat::DataFormat)m_msgBuffer[0];
	if ( DataFormat::BStruct != df )//ֻ����BStruct��ʽ�ı���
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;
	}
	if ( !msg.Resolve(&m_msgBuffer[MSG_HEAD_SIZE],len) ) 
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;//��������
	}

	//////////////////////////////////////////////////////////////////////////
	//������
	if ( !msg["msgid"].IsValid() || sizeof(unsigned short) != msg["msgid"].m_size ) //�������msgid�ֶ�
	{
		InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
		return;
	}
	unsigned short msgid = msg["msgid"];
	bool bIsValidMsg = false;//Ĭ��Ϊ�Ƿ�����
	switch( msgid )
	{
	case msgid::eNodeJoin ://������
		bIsValidMsg = OnNodeJoin(host, msg);
		break;
	case msgid::eNodeReady://������
		bIsValidMsg = OnNodeReady(host);
		break;
	case msgid::eNodeExit://����˳�
		bIsValidMsg = OnNodeExit(host);
		break;
	case msgid::uCommand://�˹�����ָ��
		bIsValidMsg = OnUserCommand(host, msg);
		break;
	case msgid::eHeartbeat://����
		bIsValidMsg = true;
		break;
	default:
		break;
	}
	if ( !bIsValidMsg ) InvalidMsg(host, m_msgBuffer, len + MSG_HEAD_SIZE);
}

//������ֲ�����
bool TCPWorker::OnNodeJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//////////////////////////////////////////////////////////////////////////
	/*
		��鱨�ĺϷ���
		1.���е��ֶ���û��
		2.�����Ƿ���Э��Լ�����
		3.�ַ����ȱ䳤�ֶγ��ȱ��벻С��1
	*/
	if ( !msg["port"].IsValid() || sizeof(unsigned short) != msg["port"].m_size ) return false;
	if ( !msg["wan_ip"].IsValid() || 1 > msg["wan_ip"].m_size ) return false;
	if ( !msg["lan_ip"].IsValid() || 1 > msg["lan_ip"].m_size ) return false;
	if ( !msg["role"].IsValid() || sizeof(unsigned char) != msg["role"].m_size ) return false;
	if ( !msg["pieceNo"].IsValid() || sizeof(unsigned int) != msg["pieceNo"].m_size ) return false;
	
	//////////////////////////////////////////////////////////////////////////
	//ҵ����
	unsigned char role = msg["role"];
	if ( NodeRole::Database == role ) //���ݿ����
	{
		DBJoin(host, msg);
		return true;
	}
	if ( NodeStatus::NotOnline == m_master.status )//ֻҪ����Ƭû���룬��һ�������Ϊ����Ƭ
	{
		MasterJoin(host, msg);
		return true;
	}
	PieceJoin(host, msg);//��Ϊ��Ƭ����

	return true;
}

//������
bool TCPWorker::OnNodeReady(mdk::STNetHost &host)
{
	if ( m_db.host.ID() == host.ID() ) 
	{
		DatabaseReady();
		return true;
	}
	if ( m_master.host.ID() == host.ID() ) 
	{
		printf( "��Ƭ����\n" );
		m_master.status = NodeStatus::Serving;
	}
	else
	{
		NodeList::iterator it = m_nodes.find( host.ID() );
		if ( it == m_nodes.end() ) return false;
		printf( "��Ƭ%d����\n", it->second->pieceNo );
		it->second->status = NodeStatus::Serving;
	}
	return true;
}

//��Ӧ����˳�����
bool TCPWorker::OnNodeExit(mdk::STNetHost &host)
{
	Exist *pExist = NULL;
	if ( m_db.host.ID() == host.ID() ) //���ݿ��˳�
	{
		pExist = &m_db;
		DBExit();
	}
	else if ( m_master.host.ID() == host.ID() ) //����Ƭ�˳�
	{
		pExist = &m_master;
		pExist->status = NodeStatus::NotOnline;
	}
	else//����˳�
	{
		NodeList::iterator it = m_nodes.find( host.ID() );
		if ( it == m_nodes.end() ) return false;
		pExist = it->second;
		pExist->status = NodeStatus::Idle;
	}
	DelNode(pExist);
	return true;
}

//��Ӧ�˹�����ָ��
bool TCPWorker::OnUserCommand(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["cmdCode"].IsValid() || sizeof(unsigned char) != msg["cmdCode"].m_size ) return false;
	unsigned char cmdCode = msg["cmdCode"];
	bool bIsInvalidMsg = true;//Ĭ��Ϊ�Ƿ�����
	switch( cmdCode )
	{
	case UserCmd::QueryNodeList :
		bIsInvalidMsg = OnQueryNodeList(host);
		break;
	case UserCmd::StartPiece :
		bIsInvalidMsg = OnStartPiece(host, msg);
		break;
	case UserCmd::NodeDel :
		bIsInvalidMsg = OnNodeDel(host, msg);
		break;
	default:
		break;
	}
	return bIsInvalidMsg;
}

//��ѯ���н��
bool TCPWorker::OnQueryNodeList(mdk::STNetHost &host)
{
	SendSystemStatus(host);
	
	return true;
}

//��ʼ��Ƭָ��
bool TCPWorker::OnStartPiece(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["pieceNo"].IsValid() || sizeof(unsigned int) != msg["pieceNo"].m_size ) return false;
	unsigned int pieceNo = msg["pieceNo"];
	char *errReason = StartPiece(pieceNo);
	ReplyUserResult(host, msg, errReason);
	return true;
}

//ɾ�����ָ��
bool TCPWorker::OnNodeDel(mdk::STNetHost &host, bsp::BStruct &msg)
{
	if ( !msg["hostID"].IsValid() || sizeof(int) != msg["hostID"].m_size ) return false;
	int hostID = msg["hostID"];
	NodeList::iterator it = m_nodes.find( host.ID() );
	char *errReason = NULL;
	if ( it != m_nodes.end() ) errReason = DelNode(it->second);
	else if ( m_db.host.ID() == host.ID() )  errReason = DelNode(&m_db);
	else if (m_master.host.ID() == host.ID())errReason = DelNode(&m_master);
	ReplyUserResult(host, msg, errReason);
	return true;
}
