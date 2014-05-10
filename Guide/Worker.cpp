// Worker.cpp: implementation of the Worker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define MAX_LEN 2000

//���ݿ���뵽������
void TCPWorker::DBJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "���ݿ������¼\n" );
	//У�������Ϣ
	if ( NodeStatus::NotOnline != m_db.status ) //���ݿ������ߣ�����У������������һ��ҵ�����̴�����ֹ�ƻ����ݿ�
	{
		printf( "�ܾ������ݿ�������\n" );
		NodeCheck(host, NodeRole::Unknow, 0, "���ݿ�������");
		unsigned short port = msg["port"];
		string wanIP = (string)msg["wan_ip"];
		string lanIP = (string)msg["lan_ip"];
		m_log.Info("info:", "Waring:refuse Database join:Database already in net, host info:id %d; wan ip%s; lan ip%s; port%d", 
			host.ID(), wanIP.c_str(), lanIP.c_str(), port );
		return;
	}
	printf( "���ݿ��¼\n" );
	NodeCheck(host, NodeRole::Database, 0, NULL);
	
	/*
		�����ip�Ƿ���֮ǰ��ƥ�䣬��Ϊip����������п��ܱ仯��
		���û�����֤��������ݿ���ͬһ̨����
	 */
	m_db.host = host;
	m_db.port = msg["port"];
	m_db.wanIP = (string)msg["wan_ip"];
	m_db.lanIP = (string)msg["lan_ip"];
	m_db.status = NodeStatus::LoadData;
	m_log.Info("info:", "Database join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port );
	return;
}

//����Ƭ���뵽������
void TCPWorker::MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "��Ƭ��¼\n" );
	NodeCheck(host, NodeRole::Master, -1, NULL);//У�������Ϣ
	m_master.host = host;
	m_master.wanIP = (string)msg["wan_ip"];
	m_master.lanIP = (string)msg["lan_ip"];
	m_master.port = msg["port"];
	m_master.status = NodeStatus::WaitDBReady;
	if ( NodeStatus::Serving == m_db.status ) 
	{
		SetDatabase(host);
		m_master.status = NodeStatus::LoadData;
	}
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) SetMaster(it->second->host);

	m_log.Info("info:","Master join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port );
	
	return;
}

//��Ƭ���뵽������
void TCPWorker::PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	printf( "��Ƭ�����¼\n" );
	//�½�һ��Exist��Ƭ
	Exist *pNewExist = new Exist;
	if ( NULL == pNewExist ) //����
	{
		printf( "�ܾ���Guide�ڴ治��\n" );
		NodeCheck(host, NodeRole::Unknow, 0, "Guide�ڴ治��");
		string wanIP = (string)msg["wan_ip"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short port = msg["port"];
		m_log.Info("info:","Waring:refuse Exist join:no enough memory, host info:id %d; wan ip%s; lan ip%s; port%d", 
			host.ID(), wanIP.c_str(), lanIP.c_str(), port );
		return;
	}
	pNewExist->host = host;
	pNewExist->wanIP = (string)msg["wan_ip"];
	pNewExist->lanIP = (string)msg["lan_ip"];
	pNewExist->port = msg["port"];
	pNewExist->role = NodeRole::Piece;
	pNewExist->status = NodeStatus::Idle;
	pNewExist->pieceNo = msg["pieceNo"];
	//���Ƭ�ų�ͻ������У��
	NodeList::iterator it = m_nodes.begin();
	for ( ; 0 <= pNewExist->pieceNo && it != m_nodes.end(); it++ )
	{
		if ( pNewExist->pieceNo == it->second->pieceNo )
		{
			pNewExist->pieceNo = -1;
			break;
		}
	}
	printf( "�������󣬷�Ƭ��%d\n", pNewExist->pieceNo );
	NodeCheck(host, NodeRole::Piece, pNewExist->pieceNo, NULL);
	/*
		mdk��֤��OnCloseConnect()���ǰ��socket������Բ��ᱻ������ʹ�ã�
		������OnCloseConnect()��ɾ��socket���
		������Բ�����ֳ�ͻʧ��
	 */
	m_nodes.insert(NodeList::value_type(host.ID(), pNewExist));
	if ( NodeStatus::NotOnline != m_db.status && NodeStatus::Closing != m_db.status ) SetDatabase(host);//�������ݿ�
	if ( NodeStatus::NotOnline != m_master.status && NodeStatus::Closing != m_master.status ) SetMaster(host);//��������Ƭ
	m_log.Info("info:", "Exist join, host info:id %d; wan ip%s; lan ip%s; port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.lanIP.c_str(), m_db.port);

	if ( 0 <= pNewExist->pieceNo && NodeStatus::Serving == m_db.status ) 
	{
		StartPiece(pNewExist->pieceNo);//��ʼ��Ƭ
	}
	
	return;
}

//����һ����Ƭ��
int TCPWorker::NewPieceNo()
{
	int pieceNo = 0;
	NodeList::iterator it;
	bool noIsExist = false;
	unsigned int maxPieceNo = CalPieceNo(0xffffffff, m_pieceSize);
	for ( pieceNo = 0; pieceNo <= maxPieceNo; pieceNo++ )
	{
		noIsExist = false;
		it = m_nodes.begin();
		for ( ; it != m_nodes.end(); it++ )
		{
			if ( pieceNo == it->second->pieceNo )
			{
				noIsExist = true;
				break;
			}
		}
		if ( !noIsExist ) break;
	}
	if (noIsExist) return -1;
	return pieceNo;
}

//���ݿ����
void TCPWorker::DatabaseReady()
{
	printf( "���ݿ����\n" );
	m_db.status = NodeStatus::Serving;
	if ( NodeStatus::WaitDBReady == m_master.status ) 
	{
		SetDatabase(m_master.host);
		m_master.status = NodeStatus::LoadData;
	}
	
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		pExist = it->second;
		if ( pExist->pieceNo < 0 || NodeStatus::Idle != pExist->status ) continue;
		StartPiece(pExist->pieceNo);//��ʼ��Ƭ
	}
	
	return;
}

//���ݿ��˳������н��ֹͣ����
void TCPWorker::DBExit()
{
	printf( "���ݿ�Ͽ�\n" );
	m_db.status = NodeStatus::NotOnline;
	if ( NodeStatus::NotOnline != m_master.status ) m_master.status = NodeStatus::WaitDBReady;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) it->second->status = NodeStatus::Idle;
}

//����һ����Ƭ���ɹ�����NULL,ʧ�ܷ���ԭ��
char* TCPWorker::StartPiece(unsigned int pieceNo)
{
	if ( NodeStatus::Serving == m_db.status ) 
	{
		printf( "�ܾ���Ƭָ����ݿ�δ����\n" );
		return "���ݿ�δ����";
	}
	
	Exist *pExist = NULL;
	Exist *pDefPiece = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		if ( NodeStatus::Idle != pExist->status ) 
		{
			if ( pieceNo == it->second->pieceNo ) 
			{
				printf( "�ܾ���Ƭָ���Ƭ%d�ѹ���\n", it->second->pieceNo );
				return "��Ƭ�ѹ���";
			}
			continue;
		}
		if ( 0 <= pieceNo )//ָ����Ƭ��
		{
			if ( pieceNo == it->second->pieceNo )
			{
				if ( NULL == pDefPiece ) pDefPiece = it->second;//��һ��δ������Ƭ����ΪĬ��
				pExist = it->second;
				break;
			}
			if ( NULL == pExist && 0 > it->second->pieceNo ) 
				pExist = it->second;//ʹ��һ���շ�Ƭ
		}
		else//ûָ��Ƭ,�������һ����Ƭ
		{
			if ( it->second->pieceNo >= 0 ) 
			{
				pExist = it->second;
				break;
			}
			if ( NULL == pExist ) pExist = it->second;//ʹ��һ���շ�Ƭ
		}
	}

	if ( NULL == pExist ) 
	{
		if ( NULL == pDefPiece ) 
		{
			printf( "�ܾ���Ƭָ��޿��н�㣬�����ӽ�㵽������\n" );
			return "�޿��н�㣬�����ӽ�㵽������";
		}
		else 
		{
			pExist = pDefPiece;
			pExist->pieceNo = pieceNo;
		}
	}
	
	if ( 0 > pExist->pieceNo ) 
	{
		pExist->pieceNo = NewPieceNo();
		if ( 0 > pExist->pieceNo ) 
		{
			printf( "�ܾ���Ƭָ���Ƭ��������ɾ��һЩ���ý��\n" );
			return "��Ƭ��������ɾ��һЩ���ý��";
		}
	}
	
	pExist->status = NodeStatus::LoadData;
	NodeInit(pExist);
	it = m_pieces.find(pieceNo);
	if ( it != m_pieces.end() )
	{
		Exist* pNotOnlineExist = it->second;
		m_pieces.erase(it);
		delete pNotOnlineExist;
	}
	m_pieces.insert(PieceList::value_type(pExist->pieceNo, pExist));//���뵽��Ƭ�б�
	m_log.Info("info:", "Start Piece, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
		pExist->pieceNo, 
		pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	return NULL;
}

//ɾ��һ����㣬�ɹ�����NULL��ʧ�ܷ���ԭ��
char* TCPWorker::DelNode(Exist *pExist)
{
	if ( &m_db == pExist || &m_master == pExist )
	{
		if ( NodeStatus::NotOnline == pExist->status ) 
		{
			if (&m_db == pExist) 
			{
				m_log.Info("info:","Delete Database, Node info:hostid %d; wan ip%s; lan ip%s; port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
			}
			else if (&m_master == pExist) 
			{
				m_log.Info("info:", "Delete Master, Node info:hostid %d; wan ip%s; lan ip%s; port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
			}
			pExist->host.Close();
			return NULL;
		}
	}
	else if ( NodeStatus::Idle == pExist->status ) 
	{
		m_nodes.erase(pExist->host.ID());
		m_pieces.erase(pExist->host.ID());
		pExist->host.Close();
		m_log.Info("info:", "Delete Node, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
		delete pExist;
		return NULL;
	}
	//ֹͣ����
	if ( NodeStatus::Closing == pExist->status ) return "����ֹͣ����ϵķ��񣬽�㽫�������ֹͣ���Զ�ɾ��";
	pExist->status = NodeStatus::Closing;
	NodeExit(pExist);//֪ͨ����˳�

	if ( NodeRole::Database == pExist->role ) 
	{
		m_log.Info("info:", "Node stop service, Node info:Database; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	else if ( NodeRole::Master == pExist->role ) 
	{
		m_log.Info("info:", "Node stop service, Node info:Master; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	else 
	{
		m_log.Info("info:", "Node stop service, Node info:pieceNo %d; hostid %d; wan ip%s; lan ip%s; port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->lanIP.c_str(), pExist->port);
	}
	return "׼��ֹͣ����ϵķ��񣬽�㽫�������ֹͣ���Զ�ɾ��";
}
