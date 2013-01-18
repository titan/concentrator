#ifndef __CLOCK_H
#define __CLOCK_H
#include"sysdefs.h"
#if __OS__ == WINDOWS
#include<windows.h>
#elif __OS__ == LINUX
#include<pthread.h>
#endif
class CLock
{
   public:
      CLock();
      ~CLock();
      void Lock();
      bool TryLock();
      void UnLock();
   private:
   #if __OS__ == WINDOWS
      CRITICAL_SECTION m_csLock;
   #elif __OS__ == LINUX
      pthread_mutex_t m_csLock;
   #endif
};
#endif
