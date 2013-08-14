#ifndef __CTIMER_H
#define __CTIMER_H
#include<time.h>
#include<string.h>
#include"sysdefs.h"

class CTimer
{
public:
    CTimer():m_LastTime(0), m_nInterval(0) {};
    bool StartTimer(uint32 Interval/*second*/);
    bool Done();
    void StopTimer() {
        m_nInterval = 0;
    };
private:
    uint32 m_LastTime;/*second*/
    uint32 m_nInterval;/*second*/
};

class CRepeatTimer
{
public:
    CRepeatTimer():m_LastTime(0), m_nInterval(0) {};
    bool Start(uint32 Interval/*second*/);
    bool Done();
    void Stop() {
        m_nInterval = 0;
    };
private:
    uint32 m_LastTime;/*second*/
    uint32 m_nInterval;/*second*/
};

enum FixTimerTypeE {
    DAY_TYPE=2,
    HOUR_TYPE,
    MINUTE_TYPE,
};
class CFixTimer
{
public:
    CFixTimer(FixTimerTypeE TimerType):m_TimerType(TimerType)
        , m_NextTime(0)
        , m_nInterval(0) {};
    bool Start(uint32 TimeOffset=0);
    bool Done();
    void Stop() {
        m_nInterval = 0;
    };
private:
    const FixTimerTypeE m_TimerType;
    uint32 m_NextTime;/*second*/
    uint32 m_nInterval;/*second*/
};
#endif
