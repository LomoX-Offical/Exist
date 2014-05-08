#ifndef COMMOND_H
#define COMMOND_H
namespace NodeStatus
{
	enum NodeStatus
	{
		Idle = 0,
		LoadData = 1,
		Serving = 2,
		Closing = 3,
		WaitDBReady = 4,
		NotOnline = 5,
	};
}

//״̬����
char* DesStatus(unsigned char status);

namespace NodeRole
{
	enum NodeRole
	{
		Unknow = 0,
		Database = 1,
		Master = 2,
		Piece = 3,
	};
}

//��ɫ����
char* DesRole(unsigned char status);

//hashid���ڼ���Ƭ��
unsigned int CalPieceNo( unsigned int hashid, unsigned int pieceSize );

#endif //COMMOND_H
