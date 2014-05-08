// Worker.cpp: implementation of the Worker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//���ݿ���뵽������
void TCPWorker::DBJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//У�������Ϣ
	if ( NodeStatus::NotOnline != m_db.status ) //���ݿ������ߣ�����У������������һ��ҵ�����̴�����ֹ�ƻ����ݿ�
	{
		NodeCheck(host, NodeRole::Unknow, 0, "���ݿ�������");
		string wanIP = (string)msg["wan_ip"];
		unsigned short wanPort = msg["wan_port"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short lanPort = msg["lan_port"];
		m_log.Run( "Waring:refuse Database join:Database already in net, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			host.ID(), wanIP.c_str(), wanPort, lanIP.c_str(), lanPort );
		return;
	}
	NodeCheck(host, NodeRole::Database, 0, NULL);
	
	/*
		�����ip�Ƿ���֮ǰ��ƥ�䣬��Ϊip����������п��ܱ仯��
		���û�����֤��������ݿ���ͬһ̨����
	 */
	m_db.wanIP = (string)msg["wan_ip"];
	m_db.wanPort = msg["wan_port"];
	m_db.lanIP = (string)msg["lan_ip"];
	m_db.lanPort = msg["lan_port"];
	m_db.status = NodeStatus::LoadData;
	m_log.Run( "Database join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort );
	return;
}

//����Ƭ���뵽������
void TCPWorker::MasterJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	NodeCheck(host, NodeRole::Master, 0, NULL);//У�������Ϣ
	m_master.wanIP = (string)msg["wan_ip"];
	m_master.wanPort = msg["wan_port"];
	m_master.lanIP = (string)msg["lan_ip"];
	m_master.lanPort = msg["lan_port"];
	m_master.status = NodeStatus::WaitDBReady;
	if ( NodeStatus::Serving == m_db.status ) 
	{
		m_master.status = NodeStatus::LoadData;
		SetDatabase(host);
	}
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) SetMaster(it->second->host);

	m_log.Run( "Master join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort );
	
	return;
}

//��Ƭ���뵽������
void TCPWorker::PieceJoin(mdk::STNetHost &host, bsp::BStruct &msg)
{
	//�½�һ��Exist��Ƭ
	Exist *pNewExist = new Exist;
	if ( NULL == pNewExist ) //����
	{
		NodeCheck(host, NodeRole::Unknow, 0, "Guide�ڴ治��");
		string wanIP = (string)msg["wan_ip"];
		unsigned short wanPort = msg["wan_port"];
		string lanIP = (string)msg["lan_ip"];
		unsigned short lanPort = msg["lan_port"];
		m_log.Run( "Waring:refuse Exist join:no enough memory, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			host.ID(), wanIP.c_str(), wanPort, lanIP.c_str(), lanPort );
		return;
	}
	pNewExist->host = host;
	pNewExist->wanIP = (string)msg["wan_ip"];
	pNewExist->wanPort = msg["wan_port"];
	pNewExist->lanIP = (string)msg["lan_ip"];
	pNewExist->lanPort = msg["lan_port"];
	pNewExist->role = NodeRole::Piece;
	pNewExist->status = NodeStatus::Idle;
	pNewExist->pieceNo = msg["pieceNo"];
	//���Ƭ�ų�ͻ������У��
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		if ( pNewExist->pieceNo == it->second->pieceNo )
		{
			pNewExist->pieceNo = -1;
			break;
		}
	}
	NodeCheck(host, NodeRole::Piece, pNewExist->pieceNo, NULL);
	/*
		mdk��֤��OnCloseConnect()���ǰ��socket������Բ��ᱻ������ʹ�ã�
		������OnCloseConnect()��ɾ��socket���
		������Բ�����ֳ�ͻʧ��
	 */
	m_nodes.insert(NodeList::value_type(host.ID(), pNewExist));
	if ( NodeStatus::NotOnline != m_db.status && NodeStatus::Closing != m_db.status ) SetDatabase(host);//�������ݿ�
	if ( NodeStatus::NotOnline != m_master.status && NodeStatus::Closing != m_master.status ) SetMaster(host);//��������Ƭ
	m_log.Run( "Exist join, host info:id %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		host.ID(), m_db.wanIP.c_str(), m_db.wanPort, m_db.lanIP.c_str(), m_db.lanPort);

	if ( 0 <= pNewExist->pieceNo && NodeStatus::Serving == m_db.status ) 
	{
		StartPiece(pNewExist->pieceNo, pNewExist->host.ID());//��ʼ��Ƭ
	}
	
	return;
}

//���ݿ����
void TCPWorker::DatabaseReady()
{
	m_db.status = NodeStatus::Serving;
	if ( NodeStatus::WaitDBReady == m_master.status ) m_master.status = NodeStatus::LoadData;
	SetDatabase(m_master.host);
	
	Exist *pExist = NULL;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		pExist = it->second;
		if ( pExist->pieceNo < 0 || NodeStatus::Idle != pExist->status ) continue;
		StartPiece(pExist->pieceNo, pExist->host.ID());//��ʼ��Ƭ
	}
	
	return;
}

//���ݿ��˳������н��ֹͣ����
void TCPWorker::DBExit()
{
	m_db.status = NodeStatus::NotOnline;
	if ( NodeStatus::NotOnline != m_master.status ) m_master.status = NodeStatus::WaitDBReady;
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ ) it->second->status = NodeStatus::Idle;
}

//����һ����Ƭ���ɹ�����NULL,ʧ�ܷ���ԭ��
char* TCPWorker::StartPiece(unsigned int pieceNo, unsigned int hostID)
{
	if ( NodeStatus::Serving == m_db.status ) return "���ݿ�δ����";
	
	Exist *pExist = NULL;
	if ( hostID >= 0 )
	{
		NodeList::iterator it = m_nodes.find( hostID );
		if ( it == m_nodes.end() ) return "ָ����㲻����";
		pExist = it->second;
		if ( NodeStatus::Idle != pExist->status ) return "ָ�����ǿ���";
	}
	else
	{
		NodeList::iterator it = m_nodes.begin();
		for ( ; it != m_nodes.end(); it++ )
		{
			if ( NodeStatus::Idle != pExist->status ) continue;
			pExist = it->second;
			//�������Ԥ�ȷ����Ƭ���Ƿ��뵱ǰָ��Ƭ����ͬ��ֱ�Ӹ��ǣ����˹�Ϊ׼
			break;
		}
	}
	if ( NULL == pExist ) return "�޿��н�㣬�����ӽ�㵽������";
	pExist->pieceNo = pieceNo;
	pExist->status = NodeStatus::LoadData;
	NodeInit(pExist);
	PieceList::iterator it = m_pieces.find(pieceNo);
	if ( it != m_pieces.end() )
	{
		Exist* pNotOnlineExist = it->second;
		m_pieces.erase(it);
		delete pNotOnlineExist;
	}
	m_pieces.insert(PieceList::value_type(pExist->pieceNo, pExist));//���뵽��Ƭ�б�
	m_log.Run( "Start Piece, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
		pExist->pieceNo, 
		pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
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
				m_log.Run( "Delete Database, Node info:hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
			}
			else if (&m_master == pExist) 
			{
				m_log.Run( "Delete Master, Node info:hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
					pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
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
		m_log.Run( "Delete Node, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
		delete pExist;
		return NULL;
	}
	//ֹͣ����
	if ( NodeStatus::Closing == pExist->status ) return "����ֹͣ����ϵķ��񣬽�㽫�������ֹͣ���Զ�ɾ��";
	pExist->status = NodeStatus::Closing;
	NodeExit(pExist);//֪ͨ����˳�

	if ( NodeRole::Database == pExist->role ) 
	{
		m_log.Run( "Node stop service, Node info:Database; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	else if ( NodeRole::Master == pExist->role ) 
	{
		m_log.Run( "Node stop service, Node info:Master; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	else 
	{
		m_log.Run( "Node stop service, Node info:pieceNo %d; hostid %d; wan ip%s; wan port%d; lan ip%s; lan port%d", 
			pExist->pieceNo, 
			pExist->host.ID(), pExist->wanIP.c_str(), pExist->wanPort, pExist->lanIP.c_str(), pExist->lanPort);
	}
	return "׼��ֹͣ����ϵķ��񣬽�㽫�������ֹͣ���Զ�ɾ��";
}

