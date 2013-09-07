#ifndef __CVALVE_MONITOR_H
#define __CVALVE_MONITOR_H

#include <vector>
#include <map>
#include "IThread.h"
#include "cbuffer.h"
#include "sysdefs.h"
#include "CTimer.h"
#include "IValveMonitor.h"
#include "CLock.h"

using namespace std;

typedef struct {
    uint32 mac;
    userid_t uid;
    uint16 indoorTemp;
    uint16 setTemp;
    uint8 openTime;
    uint64 totalTime;
    uint64 remainingTime;
    uint64 runningTime;
    uint32 timestamp;
} __attribute__((packed, aligned(1))) valve_temperature_t;

typedef struct {
    uint32 mac;
    userid_t uid;
    uint8 type;
    uint8 addr[7];
    uint8 inTemp[3];
    uint8 outTemp[3];
    uint8 totalFlow[5];
    uint8 totalHeat[5];
    uint8 workTime[3];
    uint32 timestamp;
} __attribute__ ((packed, aligned(1))) valve_heat_t;

typedef void (*expire_callback)(user_t *);

typedef struct {
    time_t timestamp;
    expire_callback callback;
} expire_t;

#ifdef DEBUG_VALVE_TRACE_RECHARGE

typedef struct {
    time_t timestamp1;
    time_t timestamp2;
    time_t timestamp3;
} mytimer_t;

#endif

class CValveMonitor: public IThread, public IValveMonitor
{
public:
    static CValveMonitor * GetInstance();
    bool Init(uint32 startTime, uint32 interval);
    void SetCom(int com) {
        this->com = com;
    };
    void SetValveDataType(ValveDataType type) {
        valveDataType = type;
    };
    ValveDataType GetValveDataType() {
        return valveDataType;
    };
    void QueryUser(userid_t uid, uint8 * data, uint16 len);
    void Recharge(userid_t uid, uint8 * data, uint16 len);
    uint16 ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len);
    Status GetStatus();
    void Broadcast(uint8 counter);
    bool GetUserList(vector<user_t>& user);
    void SetValveCount(int count) {
        this->valveCount = count;
    };

private:
    CValveMonitor();
    ~CValveMonitor();
    void LoadUsers();
    void SaveUsers();
    void LoadRecords();
    void SaveRecords();
    virtual uint32 Run();
    void ParseAck(uint8 * ack, uint16 len);
    void BroadcastClearSID();
    void BroadcastQuery();
    void ParseBroadcast(uint32 vmac, uint8 * data, uint16 len);
    void GetPunctualData();
    void ParsePunctualData(uint32 vmac, uint8 * data, uint16 len);
    void GetRechargeData();
    void ParseRechargeData(uint32 vmac, uint8 * data, uint16 len);
    void GetConsumeData();
    void ParseConsumeData(uint32 vmac, uint8 * data, uint16 len);
    void ParseQueryUser(uint32 vmac, uint8 * data, uint16 len);
    void ParseRecharge(uint32 vmac, uint8 * data, uint16 len);
    void GetTimeData();
    void ParseTimeData(uint32 vmac, uint8 * data, uint16 len);
    void SendValveData();
    void GetHeatData();
    void ParseHeatData(uint32 vmac, uint8 * data, uint16 len);
    bool SendCommand(uint8 * data, uint16 len);
    bool WaitCmdAck(uint8 * data, uint16 * len);
    void GetValveTime(uint32 vmac);
    void ParseValveTime(uint32 vmac, uint8 * data, uint16 len);
    void SetValveTime(uint32 vmac, tm * time);
    void SyncValveTime();
    void SendErrorValves();
    bool SetUID(uint32 vmac, userid_t uid);

    static CValveMonitor * instance;
    int com;
    map<uint32, user_t> users; // vmac -> user_t
    map<uint32, user_t> lastUsers; // vmac -> user_t
    map<uint32, expire_t> expires; // vmac -> expire_t
    CLock users_lock, txlock, htxlock;
    cbuffer_t tx; // send command buffer
    cbuffer_t htx; // high priority sending command buffer
    CRepeatTimer punctualTimer;
    CFixTimer noonTimer;
    map<uint32, record_t> records; // vmac -> record
    map<uint32, valve_temperature_t> valveTemperatures; // vmac -> ...
    map<uint32, valve_heat_t> valveHeats; // vmac -> ...
    ValveDataType valveDataType;
    bool syncUsers, syncRecords;
    uint32 sid; // session id
    uint8 counter; // broadcase counter
    unsigned int valveCount;
    cbuffer_t rx; // received valve macs
    int unregisteredCounter; // unregistered valve counter
#ifdef DEBUG_VALVE_TRACE_RECHARGE
    map<uint32, mytimer_t> timers;
#endif
    map<uint32, int> requestCounters; // remove me
    map<uint32, int> responseCounters; // remove me
};
#endif
