#ifndef __ITIMERTHREAD_H
#define __ITIMERTHREAD_H
#include <pthread.h>
#include <sys/time.h>
#include"sysdefs.h"
class CTimerThread
{
   public:
      CTimerThread();
      virtual ~CTimerThread();
      void SetTimer(uint32 Begin/*ms*/, uint32 nInterval/*ms*/);
      void StartTimer();
      void StopTimer();
      pthread_t GetThreadHandle(){return m_hThreadTimer;}
      bool IsStarted(){return NULL != m_hThreadTimer;}

   private:
      pthread_t m_hThreadTimer;
      uint32 m_Begin;
      uint32 m_nInterval;
      static void *OnTimer_stub(void *p)
      {
         (static_cast<CTimerThread*>(p))->thread_proc();
         return NULL;
      }
      void thread_proc();
      virtual void OnTimer();
};
#endif
