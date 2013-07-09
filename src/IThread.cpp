#include<stdio.h>
#include"IThread.h"

#ifdef DEBUG_THREAD
#include <time.h>
#define DEBUG(...) do {printf("%ld %s::%s %d ----", time(NULL), __FILE__, __func__, __LINE__);printf(__VA_ARGS__);} while(false)
#ifndef hexdump
#define hexdump(data, len) do {for (uint32 i = 0; i < (uint32)len; i ++) { printf("%02x ", *(uint8 *)(data + i));} printf("\n");} while(0)
#endif
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
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
    DEBUG("%d\n", Priority);

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
        DEBUG("fail to create thread\n");
        return false;
    }
}
