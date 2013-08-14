#ifndef __ITHREAD_H
#define __ITHREAD_H
#include"sysdefs.h"
#include"CLock.h"
class IThread
{
public:
    IThread();
    virtual ~IThread();
    virtual bool Start(int32 Priority = 10);
    pthread_t GetThreadHandle() {
        return m_hThread;
    }
    bool IsStarted() {
        return 0 != m_hThread;
    }
private:
    virtual uint32 Run()=0;
protected:
    pthread_t m_hThread;
private:
    static void* thread_handler(void* thread);
};
#endif
