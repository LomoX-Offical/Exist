// ReplyMsg.cpp: implementation of the TCPWorker class.
//
//////////////////////////////////////////////////////////////////////

#include "TCPWorker.h"
#include "../common/Protocol.h"
#include "../bstruct/include/BArray.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

//���У��ָ��
void TCPWorker::NodeCheck(mdk::STNetHost &host, unsigned char role, int pieceNo, char *reason )
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::gNodeCheck;
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
	printf( "�������ݿ�\n" );
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::gSetDatabase;
	msg["wan_ip"] = m_db.wanIP.c_str();
	msg["lan_ip"] = m_db.lanIP.c_str();
	msg["port"] = m_db.port;
	SendBStruct( host, msg );
}

//��������Ƭ
void TCPWorker::SetMaster(mdk::STNetHost &host)
{
	printf( "������Ƭ\n" );
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::gSetMaster;
	msg["wan_ip"] = m_master.wanIP.c_str();
	msg["lan_ip"] = m_master.lanIP.c_str();
	msg["port"] = m_master.port;
	SendBStruct( host, msg );
}

//֪ͨexist����ʼ����Ƭ
void TCPWorker::NodeInit(Exist *pExist)
{
	printf( "֪ͨ��Ƭ%d׼������\n", pExist->pieceNo );
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::gCreatePiece;
	msg["pieceNo"] = pExist->pieceNo;
	SendBStruct( pExist->host, msg );
}

//֪ͨexist����˳�����
void TCPWorker::NodeExit(Exist *pExist )
{
	bsp::BStruct msg;
	msg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
	msg["msgid"] = (unsigned short)msgid::gExitNet;
	SendBStruct( pExist->host, msg );
}

/*
	�û�����ָ̨���������ظ�
	reason=NULL��ʾ�����ɹ�
	msg�ûظ���Ӧ�Ĳ���ָ��
*/
void TCPWorker::ReplyUserResult(mdk::STNetHost &host, bsp::BStruct &msg, char *reason)
{
	unsigned char cmdCode = msg["cmdCode"];
	bsp::BStruct replyMsg;
	if ( UserCmd::StartPiece == cmdCode )
	{
		unsigned int pieceNo = msg["pieceNo"];
		int hostID = msg["hostID"];
		replyMsg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
		replyMsg["cmdCode"] = (unsigned char)UserCmd::StartPiece;
		replyMsg["pieceNo"] = pieceNo;
		replyMsg["hostID"] = hostID;
	}
	else if ( UserCmd::NodeDel == cmdCode )
	{
		int hostID = msg["hostID"];
		replyMsg.Bind( &m_msgBuffer[MSG_HEAD_SIZE], MAX_BSTRUCT_SIZE );
		replyMsg["cmdCode"] = (unsigned char)UserCmd::NodeDel;
		replyMsg["hostID"] = hostID;
	}
	replyMsg["msgid"] = (unsigned short)msgid::gCommandResult;
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
	msg["msgid"] = (unsigned short)msgid::gSystemStatus;
	bsp::BStruct db;
	db.Bind(msg.PreBuffer("database"),msg.PreSize());
	db["hostID"] = m_db.host.ID();
	db["status"] = (unsigned char)m_db.status;
	db["wan_ip"] = m_db.wanIP.c_str();
	db["lan_ip"] = m_db.lanIP.c_str();
	db["port"] = m_db.port;
	msg["database"] = &db;

	bsp::BStruct master;
	master.Bind(msg.PreBuffer("master"),msg.PreSize());
//	master["hostID"] = m_master.host.ID();
//	master["status"] = (unsigned char)m_master.status;
//	master["wan_ip"] = m_master.wanIP.c_str();
//	master["lan_ip"] = m_master.lanIP.c_str();
//	master["port"] = m_master.port;
	master["hostID"] = 2;
	master["status"] = (unsigned char)2;
	master["wan_ip"] = "127.0.0.2";
	master["lan_ip"] = "255.255.255.2";
	master["port"] = (unsigned short)2;
	msg["master"] = &master;
	bsp::BArray pieceList;
	pieceList.Bind(msg.PreBuffer("pieceList"),msg.PreSize());
	
	bsp::BStruct piece;
	int i = 0;
	PieceList::iterator itPiece = m_pieces.begin();
	Exist* pExist = NULL;
//	for ( ; itPiece != m_pieces.end(); itPiece++ )
	for ( i = 0; i < 2; )
	{
		pExist = itPiece->second;
		piece.Bind(pieceList.PreBuffer(),pieceList.PreSize());
//		piece["hostID"] = pExist->host.ID();
//		piece["status"] = (unsigned char)pExist->status;
//		piece["pieceNo"] = pExist->pieceNo;
//		piece["wan_ip"] = pExist->wanIP.c_str();
//		piece["lan_ip"] = pExist->lanIP.c_str();
//		piece["port"] = pExist->port;
		piece["hostID"] = 10 + i;
		piece["status"] = (unsigned char)0;
		piece["pieceNo"] = 20 + i;
		piece["wan_ip"] = "192.168.0.1";
		piece["lan_ip"] = "255.255.255.1";
		piece["port"] = (unsigned short)2;
		
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
		piece["lan_ip"] = pExist->lanIP.c_str();
		piece["port"] = pExist->port;
		pieceList[i++] = &piece;
	}
	msg["pieceList"] = &pieceList;
	SendBStruct( host, msg );
}
