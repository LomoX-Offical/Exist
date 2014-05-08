// RHTable.h: interface for the RHTable class.
//
//Զ�̹�ϡ��
//hash�㷨������Զ�˳������ã�ֱ�Ӵ���int��hashֵ��hash��������
//��ʵ��hashֵ�ķֲ�ʽ����
//
//��ʹ��Զ��ģʽ������redis���ù�ϡ��dict.c/h�����ƣ���Ч�ʸ���
//
//ʹ��Զ��ģʽʵ��Nosql���������ɴ����߲�ѯЧ��
//	����ѡ�õ�hash�㷨��ͬ�����Ч����ͬ
//	ѡ��md5�㷨����ѯЧ��Ϊ��ͨģʽ4��
//	ѡ��sha1�㷨����ѯЧ��Ϊ��ͨģʽ9��
//
//////////////////////////////////////////////////////////////////////

#ifndef HASHTABLE_H
#define HASHTABLE_H
#include "../Micro-Development-Kit/include/mdk/FixLengthInt.h"

#ifndef NULL
#define NULL 0
#endif

#ifdef WIN32
#include <windows.h>
#else
#endif
#define MAX_HASH_64 0x7fffffffffffffff
#define MAX_HASH_32 0x7fffffff
typedef	void (*pHashFunction)(unsigned char *hashKey, unsigned int &hashSize, unsigned char *key, unsigned int size );

class RHTable  
{
	struct ELEMENT;
	struct HASH_TABLE;
public:
//�������
typedef struct OP_R
{
	ELEMENT *pInsert;
	bool bSuccess;
}OP_R;

//������
typedef struct Iterator
{
	Iterator& operator++();
	Iterator& operator++(int);
	Iterator& operator--();
	Iterator& operator--(int);
	bool operator==(Iterator it);
	bool operator!=(Iterator it);
	ELEMENT *pElement;
private:
	friend class RHTable;
	unsigned int idx;
	HASH_TABLE *pHashTable;
	HASH_TABLE *pHeadHashTable;
}Iterator;

private:
//Ԫ��
typedef struct ELEMENT
{
	unsigned int hashValue;
	unsigned char *key;
	unsigned short keySize;
	void *value;
private:
	friend class RHTable;
	friend struct Iterator;
	bool isDel;
	ELEMENT *next;//ͬһ��Ͱ����һ��Ԫ��
}ELEMENT;

//Ͱ�����淢��hash��ײ��Ԫ��
typedef struct BUCKET
{
	ELEMENT *head;//Ͱ��Ԫ������ͷ
}BUCKET;

//��ϡ��
typedef struct HASH_TABLE
{
	BUCKET *buckets;//hash����
	unsigned long size;//hash�����С
	unsigned long sizemask;//����size��ȫ1��ʾ
	HASH_TABLE *pre;//ǰһ����
	HASH_TABLE *next;//��һ������hash��
	unsigned long count;//ʵ��Ԫ������
}HASH_TABLE;

public:
	RHTable();
	RHTable( unsigned long size );
	virtual ~RHTable();
	void SetHashFunction( pHashFunction hf );
	unsigned int RemoteHash(unsigned char *key, unsigned int size);//Զ�̼���hash����client��Serverģʽ
	void SetRemoteMode( bool bRemote );
	
public:
	OP_R* Insert(unsigned char *key, unsigned int size, void *value, unsigned int hashValue = 0);
	void* Find(unsigned char *key, unsigned int size, unsigned int hashValue = 0 );
	bool Update(unsigned char *key, unsigned int size, void *value, unsigned int hashValue = 0);
	void Delete(unsigned char *key, unsigned int size, unsigned int hashValue = 0);
	unsigned long Count();
	bool IsEmpty();
	void Clear();
	Iterator Begin();
	Iterator End();
public:
	unsigned long NextPower(unsigned long size);//��size�����С��2��n������
	unsigned int DJBHash(const unsigned char *buf, int len);//C33�㷨hashת������
	bool KeyCmp( unsigned char *key1, int size1, unsigned char *key2, int size2 );//��ͬ����true
	bool Expand(unsigned long size);
	ELEMENT* Find(unsigned char *key, unsigned int size, unsigned int hashValue, bool bInsert );
	void ReleaseOldHashTable();//��hash�����Ϊnull���ͷ�
	void ReleaseHashTable();
	//��ϡ�㷨����ָ�루Ĭ��md5��
	void (*HashFunction)(unsigned char *hashKey, unsigned int &hashSize, 
		unsigned char *key, unsigned int size );		
	
private:
	HASH_TABLE *m_pHashTable;
	bool m_onBit64;
	unsigned long m_maxHash;
	Iterator m_it;
	bool m_bRemote;
};

#endif // !defined(HASHTABLE_H)
