#include<stdio.h>
#include"IThread.h"

#ifdef DEBUG_THREAD
#define DEBUG printf
#else
#define DEBUG(...)
#endif

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
    DEBUG("IThread::Start()\n");

    pthread_create(&m_hThread, 0, thread_handler, this);
    if (m_hThread) {
        pthread_attr_t attr;
        int rs;

        rs = pthread_attr_init(&attr);
        if (rs == 0) {
            struct sched_param param;
            param.__sched_priority = Priority % 100;
            pthread_attr_setschedparam(&attr, &param);
        }
        return true;
    } else {
        DEBUG("IThread::Start()-----fail to create thread\n");
        return false;
    }
}
