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

struct uidcmp {
    bool operator() (const userid_t& lhs, const userid_t& rhs) const { for (int i = 0; i < USERID_LEN; i ++) {if (lhs.x[i] == rhs.x[i]) continue; else return lhs.x[i] < rhs.x[i];} return false;}
};

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

class CValveMonitor: public IThread, public IValveMonitor {
public:
    static CValveMonitor * GetInstance();
    bool Init(uint32 startTime, uint32 interval);
    void SetCom(int com) {this->com = com;};
    void SetValveDataType(ValveDataType type){ valveDataType = type;};
    ValveDataType GetValveDataType(){return valveDataType;};
    void QueryUser(userid_t uid, uint8 * data, uint16 len);
    void Recharge(userid_t uid, uint8 * data, uint16 len);
    uint16 ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len);
    Status GetStatus();
    void Broadcast();
    bool GetUserList(vector<user_t>& user);

private:
    CValveMonitor();
    ~CValveMonitor();
    void LoadUsers();
    void SaveUsers();
    void LoadRecords();
    void SaveRecords();
    virtual uint32 Run();
    void ParseAck(uint8 * ack, uint16 len);
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

    static CValveMonitor * instance;
    int com;
    map<userid_t, user_t, uidcmp> users;
    CLock users_lock, txlock;
    cbuffer_t tx;
    CRepeatTimer punctualTimer;
    CFixTimer noonTimer;
    map<uint32, record_t> records; // vmac -> record
    map<uint32, valve_temperature_t> valveTemperatures; // vmac -> ...
    map<uint32, valve_heat_t> valveHeats; // vmac -> ...
    ValveDataType valveDataType;
    bool syncUsers, syncRecords;
};
#endif
