#ifndef __CKEY_H
#define __CKEY_H
#include"IThread.h"
#include"CLock.h"

class CKey:public IThread
{
   public:
      static CKey* GetInstance();
   private:
      CKey(){}
      static CKey* m_Instance;
      static CLock m_Lock;
   private:
      virtual uint32 Run();
};
#endif
