#ifndef _IVALVE_MONITOR_H
#define _IVALVE_MONITOR_H
#include <vector>
#include "sysdefs.h"

using namespace std;

enum ValveDataType {
    VALVE_DATA_TYPE_HEAT,
    VALVE_DATA_TYPE_TEMPERATURE,
    VALVE_DATA_TYPE_MAX//never delete this
};

enum ValveCtrlType {
    VALVE_NULL=0x00,
    VALVE_SET_TIME=0x01,
    VALVE_GET_TIME=0x02,
    VALVE_GET_VALVE_RECORD=0x03,
    VALVE_GET_RECHARGE_DATA=0x04,
    VALVE_GET_CONSUME_DATA=0x05,
    VALVE_GET_TEMPERATURE=0x06,
    VALVE_SET_HEAT_TIME=0x07,
    VALVE_GET_TIME_DATA=0x08,
    VALVE_CONFIG=0x0A,
    VALVE_SWITCH_VALVE=0x0E,
    VALVE_GET_HEAT_DATA=0x11,
    VALVE_GET_PUNCTUAL_DATA=0x17,
    VALVE_QUERY_USER=0x18,
    VALVE_RECHARGE=0x19,
    VALVE_SET_UID=0x1A,
    VALVE_BROADCAST_CLEAR_SID=0xF0,
    VALVE_BROADCAST=0xFF,
    VALVE_MAX//never delete this
};

class IValveMonitor
{
public:
    virtual bool GetUserList(vector<user_t>& user) = 0;
    virtual void QueryUser(userid_t uid, uint8 * data, uint16 len) = 0;
    virtual void Recharge(userid_t uid, uint8 * data, uint16 len) = 0;
    virtual ValveDataType GetValveDataType() = 0;
    virtual uint16 ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len) = 0;
    virtual Status GetStatus() = 0;
};

#endif
