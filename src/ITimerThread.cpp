#include <pthread.h>
#include<stdio.h>
#include"ITimerThread.h"

#ifndef CFG_TIMER_LIB
#define CFG_TIMER_LIB
#endif
#include"libs_emsys_odm.h"

#ifdef DEBUG_TIMER_THREAD
#define DEBUG printf
#else
#define DEBUG(...)
#endif

CTimerThread::CTimerThread():m_hThreadTimer(NULL), m_Begin(0), m_nInterval(0)
{
}
CTimerThread::~CTimerThread()
{
}
void CTimerThread::SetTimer(uint32 Begin, uint32 nInterval)
{
  DEBUG("CTimerThread::SetTimer(Begin=%d, nInterval=%d)\n", Begin, nInterval);
  m_Begin = Begin;
  m_nInterval = nInterval;
}
void CTimerThread::StartTimer()
{
  DEBUG("CTimerThread::StartTimer()\n");
  pthread_create(&m_hThreadTimer, NULL, OnTimer_stub, this);
}
void CTimerThread::StopTimer()
{
  DEBUG("CTimerThread::StopTimer()\n");
  pthread_cancel(m_hThreadTimer);
  pthread_join(m_hThreadTimer, NULL); //wait the thread stopped
}
void CTimerThread::thread_proc()
{
  DEBUG("CTimerThread::thread_proc()\n");
  Usermsdelay(m_Begin);
  while(true)
  {
    DEBUG("CTimerThread::thread_proc()---loop\n");
    OnTimer();
    pthread_testcancel();
    Usermsdelay(m_nInterval);
  }
}
void CTimerThread::OnTimer()
{
  return;
}
