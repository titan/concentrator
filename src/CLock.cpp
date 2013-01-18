#include"CLock.h"
CLock::CLock()
{
  pthread_mutex_init(&(m_csLock),NULL);
}

CLock::~CLock()
{
  pthread_mutex_destroy(&(m_csLock));
}

void CLock::Lock()
{
  pthread_mutex_lock(&(m_csLock));
}

bool CLock::TryLock()
{
  return 0 == pthread_mutex_trylock(&(m_csLock));
}

void CLock::UnLock()
{
  pthread_mutex_unlock(&m_csLock);
}
