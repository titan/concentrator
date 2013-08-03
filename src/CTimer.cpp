#include<assert.h>
#include"Utils.h"
#include"CTimer.h"

#ifdef DEBUG_TIMER
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

/*********************************CTimer****************************************/
bool CTimer::StartTimer(uint32 Interval/*second*/)
{
    StopTimer();

    m_nInterval = Interval;
    if (0 == m_nInterval) {
        return false;
    }
    if (false == GetLocalTimeStamp(m_LastTime)) {
        return false;
    }

    return true;
}

bool CTimer::Done()
{
    if (0 == m_nInterval) {
        return false;
    }

    uint32 CurrentTime = 0;
    if (false == GetLocalTimeStamp(CurrentTime )) {
        return false;
    }
    if ( (CurrentTime-m_LastTime) > m_nInterval ) {
        StopTimer();
        return true;
    }

    return false;
}
/*********************************CRepeatTimer****************************************/
bool CRepeatTimer::Start(uint32 Interval/*second*/)
{
    m_nInterval = Interval;
    if (0 == m_nInterval) {
        return false;
    }
    if (false == GetLocalTimeStamp(m_LastTime)) {
        return false;
    }

    return true;
}

bool CRepeatTimer::Done()
{
    if (0 == m_nInterval) {
        return false;
    }

    uint32 CurrentTime = 0;
    if (false == GetLocalTimeStamp(CurrentTime )) {
        return false;
    }
    if ( (CurrentTime-m_LastTime) > m_nInterval ) {
        m_LastTime = CurrentTime;
        return true;
    }

    return false;
}
/*********************************CFixTimer****************************************/
bool CFixTimer::Start(uint32 TimeOffset)
{
    uint32 TimeData[DATETIME_LEN] = {0};
    if (ERROR_OK != GetDateTime(TimeData, 1)) { //RTC
        DEBUG("Can't read time from RTC\n");
        return false;
    }

    m_nInterval = 0;
    switch (m_TimerType) {
    case DAY_TYPE:
        m_nInterval = 24*60*60;//second
        break;

    case HOUR_TYPE:
        m_nInterval = 60*60;//second
        break;

    case MINUTE_TYPE:
        m_nInterval = 60;//second
        break;

    default:
        assert(0);
        return false;
    }
    if (0 == m_nInterval ) {
        DEBUG("m_nInterval=0\n");
        return false;
    }


    for (uint32 i = m_TimerType+2; i < DATETIME_LEN; i++) {
        TimeData[i] = 0;
    }
    TimeData[m_TimerType+1] = TimeOffset;
    DEBUG("NextTime=%04u-%02u-%02u %02u:%02u:%02u\n", TimeData[0], TimeData[1], TimeData[2], TimeData[3], TimeData[4], TimeData[5]);
    m_NextTime = DateTime2TimeStamp(TimeData[0], TimeData[1], TimeData[2], TimeData[3], TimeData[4], TimeData[5]);
    return true;
}

bool CFixTimer::Done()
{
    if (0 == m_nInterval ) {
        DEBUG("m_nInterval=0\n");
        return false;
    }

    uint32 CurrentTime = 0;
    if (false == GetLocalTimeStamp(CurrentTime )) {
        return false;
    }
    if (CurrentTime >= m_NextTime) {
        m_NextTime = m_NextTime+m_nInterval;
        return true;
    }
    return false;
}
