// ReplyMsg.cpp: implementation of the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//����BStruct��ʽ�ı���
bool TCPWorker::SendBStruct( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char *buf = msg.GetStream() - MSG_HEAD_SIZE;
	bsp::Stream head;
	head.Bind(buf, MSG_HEAD_SIZE);
	if ( !head.AddData((unsigned char)DataFormat::BStruct) ) return false;
	if ( !head.AddData((unsigned int)msg.GetSize()) ) return false;
	return host.Send( buf, MSG_HEAD_SIZE + msg.GetSize() );
}

//���У��ָ��
void TCPWorker::NodeCheck(mdk::STNetHost &host, unsigned char role, unsigned pieceNo, char *reason )
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gNodeCheck;
	msg["role"] = role;
	if ( NodeRole::Unknow == role ) //����ɫ�Ƿ�������ԭ��
	{
		if ( NULL == reason ) msg["reason"] = "δ֪ԭ��";
		else msg["reason"] = reason;
	}
	else if ( NodeRole::Piece == role ) msg["pieceNo"] = pieceNo;//����ɫΪ��Ƭ������Ƭ��
	SendBStruct( host, msg );
}

//�������ݿ�
void TCPWorker::SetDatabase(mdk::STNetHost &host)
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gSetDatabase;
	msg["wan_ip"] = m_db.wanIP.c_str();
	msg["wan_port"] = m_db.lanPort;
	msg["lan_ip"] = m_db.lanIP.c_str();
	msg["lan_port"] = m_db.lanPort;
	SendBStruct( host, msg );
}

//��������Ƭ
void TCPWorker::SetMaster(mdk::STNetHost &host)
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gSetMaster;
	msg["wan_ip"] = m_master.wanIP.c_str();
	msg["wan_port"] = m_master.lanPort;
	msg["lan_ip"] = m_master.lanIP.c_str();
	msg["lan_port"] = m_master.lanPort;
	SendBStruct( host, msg );
}

//֪ͨexist����ʼ����Ƭ
void TCPWorker::NodeInit(Exist *pExist)
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gCreatePiece;
	msg["pieceNo"] = pExist->pieceNo;
	SendBStruct( pExist->host, msg );
}

//֪ͨexist����˳�����
void TCPWorker::NodeExit(Exist *pExist )
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gExitNet;
	SendBStruct( pExist->host, msg );
}

/*
	�û�����ָ̨���������ظ�
	reason=NULL��ʾ�����ɹ�
	msg�ûظ���Ӧ�Ĳ���ָ��
*/
void TCPWorker::ReplyUserResult(mdk::STNetHost &host, bsp::BStruct &msg, char *reason)
{
	unsigned char cmdCode = msg["cmdcode"];
	bsp::BStruct replyMsg;
	if ( UserCmd::StartPiece == cmdCode )
	{
		unsigned int pieceNo = msg["pieceNo"];
		unsigned int hostID = msg["hostID"];
		replyMsg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
		replyMsg["pieceNo"] = pieceNo;
		replyMsg["hostID"] = hostID;
	}
	else if ( UserCmd::NodeDel )
	{
		unsigned int hostID = msg["hostID"];
		replyMsg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
		replyMsg["hostID"] = hostID;
	}
	replyMsg["msgid"] = msgid::gCommandResult;
	if (NULL == reason)replyMsg["success"] = (unsigned char)true;
	else 
	{
		replyMsg["success"] = (unsigned char)false;
		replyMsg["reason"] = reason;
	}
	SendBStruct( host, replyMsg );
}

//����ϵͳ״̬�����н����Ϣ��
void TCPWorker::SendSystemStatus(mdk::STNetHost &host)
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = msgid::gSystemStatus;
	bsp::BStruct db;
	db.Bind(msg.PreBuffer("database"),msg.PreSize());
	db["hostID"] = m_db.host.ID();
	db["status"] = (unsigned char)m_db.status;
	db["wan_ip"] = m_db.wanIP.c_str();
	db["wan_port"] = m_db.wanPort;
	db["lan_ip"] = m_db.lanIP.c_str();
	db["lan_port"] = m_db.lanPort;
	msg["database"] = &db;
	bsp::BStruct master;
	master.Bind(msg.PreBuffer("master"),msg.PreSize());
	master["hostID"] = m_master.host.ID();
	master["status"] = (unsigned char)m_master.status;
	master["wan_ip"] = m_master.wanIP.c_str();
	master["wan_port"] = m_master.wanPort;
	master["lan_ip"] = m_master.lanIP.c_str();
	master["lan_port"] = m_master.lanPort;
	msg["master"] = &master;
	bsp::BArray pieceList;
	master.Bind(msg.PreBuffer("pieceList"),msg.PreSize());
	
	bsp::BStruct piece;
	int i = 0;
	PieceList::iterator itPiece = m_pieces.begin();
	Exist* pExist = NULL;
	for ( ; itPiece != m_pieces.end(); itPiece++ )
	{
		pExist = itPiece->second;
		piece.Bind(pieceList.PreBuffer(),pieceList.PreSize());
		piece["hostID"] = pExist->host.ID();
		piece["status"] = (unsigned char)pExist->status;
		piece["pieceNo"] = pExist->pieceNo;
		piece["wan_ip"] = pExist->wanIP.c_str();
		piece["wan_port"] = pExist->wanPort;
		piece["lan_ip"] = pExist->lanIP.c_str();
		piece["lan_port"] = pExist->lanPort;
		pieceList[i++] = &piece;
	}
	NodeList::iterator it = m_nodes.begin();
	for ( ; it != m_nodes.end(); it++ )
	{
		pExist = it->second;
		itPiece = m_pieces.find(pExist->pieceNo);
		if ( itPiece != m_pieces.end() && pExist == itPiece->second ) continue;
		piece.Bind(pieceList.PreBuffer(),pieceList.PreSize());
		piece["hostID"] = pExist->host.ID();
		piece["status"] = (unsigned char)pExist->status;
		piece["wan_ip"] = pExist->wanIP.c_str();
		piece["wan_port"] = pExist->wanPort;
		piece["lan_ip"] = pExist->lanIP.c_str();
		piece["lan_port"] = pExist->lanPort;
		pieceList[i++] = &piece;
	}
	msg["pieceList"] = &pieceList;
	SendBStruct( host, msg );
}
