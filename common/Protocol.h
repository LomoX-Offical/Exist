// Protocl.h: interface for the Protocl class.
//
//////////////////////////////////////////////////////////////////////

#ifndef PROTOCOL_H
#define PROTOCOL_H

#include "../Micro-Development-Kit/include/frame/netserver/STNetHost.h"
#include "../bstruct/include/BStruct.h"

#define MAX_BSTRUCT_SIZE	1048576 //BStruct�ṹ��󳤶�1M
#define MAX_MSG_SIZE		MAX_BSTRUCT_SIZE + MSG_HEAD_SIZE//��Ϣ������󳤶ȣ�����ͷ+BStruct����С

//���ݸ�ʽ
namespace DataFormat
{
	enum DataFormat
	{
		Stream = 0,//����ʽ
		BStruct = 1,//BStruct��ʽ
	};
}

//����ͷ
typedef struct MSG_HEAD
{
	DataFormat::DataFormat dataFormat;//���ݸ�ʽ(unsigned char)
	unsigned int dataSize;//���ݳ���
}MSG_HEAD;

//����ͷ����
#define MSG_HEAD_SIZE sizeof(unsigned char)+sizeof(unsigned int)

//����id
namespace msgid
{
	enum msgid
	{
		unknow = 0,//δ֪����
		eHeartbeat = 1,//����
		eNodeJoin = 2,//exist�ڵ��������ֲ�ʽϵͳ��,У������¼ip����ɫ��Ƭ�ŵ���Ϣ,��ӦgNodeCheck����У��
		gNodeCheck = 3,//���У��ָ��
		gSetDatabase = 4,//�������ݿ⣬�ý��ȷ�����ݿ�
		gSetMaster = 5,//��������Ƭ���ý��ȷ������Ƭ
		uCommand = 6,//�˹�����ָ��
		gCommandResult = 7,//ָ��ִ�н��
		gCreatePiece = 8,//֪ͨ��㴴����Ƭ
		gExitNet = 9,//֪ͨ����˳�����
		eNodeExit = 10,//����˳���������
		gSystemStatus = 11,//ϵͳ״̬��Ϣ֪ͨ
		eNodeReady = 12,//������֪ͨ
		mQueryExistData = 13,//��ѯExist����
	};
}

//�˹�����ָ��
namespace UserCmd
{
	enum UserCmd
	{
		unknow = 0,//δָ֪��
		QueryNodeList = 1,//��ѯ���н��
		StartPiece = 2,//��ʼ��Ƭ
		NodeDel = 3,//ɾ�����
	};
}


//����BStruct��ʽ�ı���
inline bool SendBStruct( mdk::STNetHost &host, bsp::BStruct &msg )
{
	unsigned char *buf = msg.GetStream();
	buf -= MSG_HEAD_SIZE;
	bsp::Stream head;
	head.Bind(buf, MSG_HEAD_SIZE);
	if ( !head.AddData((unsigned char)DataFormat::BStruct) ) return false;
	if ( !head.AddData((unsigned int)msg.GetSize()) ) return false;

	return host.Send( buf, MSG_HEAD_SIZE + msg.GetSize() );
}


#endif // !defined(PROTOCOL_H)
