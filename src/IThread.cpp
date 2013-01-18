#include<stdio.h>
#include"IThread.h"
/*******************************IThread*******************************************/
void* IThread::thread_handler(void* thread)
{
   IThread* myThread = static_cast<IThread*>(thread);
	return (void*)myThread->Run();
}

IThread::IThread(): m_hThread(NULL)
{
}

IThread::~IThread()
{
   pthread_join(m_hThread,NULL);
   m_hThread = NULL;
}

bool IThread::Start(int32 Priority)
{
   printf("IThread::Start()\n");

	pthread_create(&m_hThread, 0, thread_handler, this);
	if(m_hThread)
	{
      return true;
	}else
   {
      printf("IThread::Start()-----fail to create thread\n");
      return false;
   }
}
