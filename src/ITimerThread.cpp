#include <pthread.h>
#include<stdio.h>
#include"ITimerThread.h"

#ifdef DEBUG_TIMER_THREAD
#define DEBUG(...) do {printf("%s::%s----", __FILE__, __func__);printf(__VA_ARGS__);} while(false)
#ifndef hexdump
#define hexdump(data, len) do {for (uint32 i = 0; i < (uint32)len; i ++) { printf("%02x ", *(uint8 *)(data + i));} printf("\n");} while(0)
#endif
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
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
