// Console.h: interface for the Console class.
//
//////////////////////////////////////////////////////////////////////

#if !defined CONSOLE_H
#define CONSOLE_H

#include <string>
#include <vector>
#include "../Micro-Development-Kit/include/mdk/Thread.h"
#include "../Micro-Development-Kit/include/mdk/Lock.h"


/*
 *	����̨��
 *	���ܼ������룬ִ��ָ��
 */
class Console
{
public:
	/*
		prompt ��������ʾ�������ΪNULL��""����ʾ����ӡ�κ���Ϣ��ֻ�ǽ������룬ִ��ָ��
		cmdMaxSize	ָ����󳤶�
	*/
	Console(char *prompt, int cmdMaxSize);
	virtual ~Console();

	/*
		��ʼ����̨
		method	���Ա��ָ��������
		pObj	method���������
		fun		ȫ�ֵ�ָ��������

		ָ������������ԭ��
		char* ExecuteCommand(std::vector<std::string> *cmd);
		cmdָ��+�����б���int main(int argc, char **argv)��argv����˳����ͬ
	*/
	bool Start( mdk::MethodPointer method, void *pObj );
	bool Start( mdk::FuntionPointer fun );
	void WaitStop();//�ȴ�����ֹ̨ͣ
	void Stop();
	void Print( const char* info );//��ӡ��Ϣ���Ȼ���
	void Line();//��ӡ����
private:
	void* RemoteCall InputThread(void*);//ָ������/ִ���߳�
	char* ExecuteCommand(char *cmd);//ִ��ָ��
	void PrintBuf();//��ӡ����̨��ʼǰ�����ܵ�������Ϣ
	void* RemoteCall KeyBroadThread(void*);//�����߳�
		
private:
	bool m_bRun;
	std::string m_prompt;//��������ʾ��
	mdk::Thread m_keyBroadThread;
	mdk::Thread m_inputThread;//�����߳�
	int m_cmdMaxSize;//ָ����󳤶�
	bool m_printInfo;
	std::vector<std::string> m_cmd;
	std::vector<std::string> m_waitPrint;
	
	bool m_isMemberMethod;
	mdk::MethodPointer m_method;
	void *m_pObj;
	mdk::FuntionPointer m_fun;
	mdk::Mutex m_printLock;
	mdk::Mutex m_ctrlLock;	
};

#endif // CONSOLE_H
