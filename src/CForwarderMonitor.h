#ifndef __CFORWARDERMONITOR_H
#define __CFORWARDERMONITOR_H
#include<map>
#include<vector>
#include<list>
#include"IThread.h"
#include"CTimer.h"
#include"CSerialComm.h"
#include"CLock.h"
#include"sysdefs.h"
using namespace std;
enum ForwarderTypeE
{
   FORWARDER_TYPE_HEAT,
   FORWARDER_TYPE_TEMPERATURE,
   FORWARDER_TYPE_MAX//never delete this
};

const uint8 FORWARDER_COMMAND_ID_LEN = sizeof(uint8);
const uint8 FORWARDER_ID_LEN = sizeof(uint32);
const uint8 VALVE_ID_LEN = sizeof(uint16);
const uint8 VALVE_CTRL_LEN = sizeof(uint32);
const uint8 VALVE_COUNT_LEN = sizeof(uint8);

const uint8 FORWARDER_COMMAND_ID_POS = 0;
const uint8 FORWARDER_ID_POS = FORWARDER_COMMAND_ID_POS+FORWARDER_COMMAND_ID_LEN;

//there is no ForwarderID in A1 ack
const uint8 A1_ACK_COMPOSITE_VALVE_COUNT_POS = FORWARDER_ID_POS+FORWARDER_ID_LEN;

const uint8 A2_VALVE_COUNT_POS = FORWARDER_ID_POS+FORWARDER_ID_LEN;
const uint8 A2_VALVE_ID_POS = A2_VALVE_COUNT_POS+VALVE_COUNT_LEN ;

const uint8 A3_VALVE_ID_POS = FORWARDER_ID_POS+FORWARDER_ID_LEN;
const uint8 A3_VALVE_CTRL_POS = A3_VALVE_ID_POS+VALVE_ID_LEN;
const uint8 A3_VALVE_VALUE_POS = A3_VALVE_CTRL_POS+VALVE_CTRL_LEN;



/*************************************valve data***************************/
const uint8 MACADDRESS_LEN = 4;
const uint8 USERID_LEN = 8;
const uint8 FORWARDER_DATETIME_LEN = 4;
const uint32 VALVE_SET_HEAT_TIME_LEN = 6*3;
const uint32 VALVE_CONFIG_LEN = 17;
/*****************************Temperature***********************************/
const uint8 FORWARDER_TEMPERATURE_LEN = 2;
const uint8 FORWARDER_TIME_LEN = 8;
const uint8 REMAINNINGTIME_LEN = 2;
struct ValveTemperatureT
{
   uint8 MacAddress[MACADDRESS_LEN];
   uint8 UserID[USERID_LEN];
   uint8 IndoorTemperature[FORWARDER_TEMPERATURE_LEN];
   uint8 SetTemperature[FORWARDER_TEMPERATURE_LEN];
   uint8 ValveRunningTime;
   uint8 ValveTotalTime[FORWARDER_TIME_LEN];
   uint8 RemainingTime[FORWARDER_TIME_LEN];
   uint8 RunningTime[FORWARDER_TIME_LEN];
   uint8 DateTime[FORWARDER_DATETIME_LEN];
}__attribute__( ( packed, aligned(1) ) );
enum ValveTemperatureTypeE
{
   ITEM_TEMPERATURE_MAC_ADDRESS,
   ITEM_TEMPERATURE_USER_ID,
   ITEM_TEMPERATURE_INDOOR_TEMPERATURE,
   ITEM_TEMPERATURE_SET_TEMPERATURE,
   ITEM_TEMPERATURE_VALVE_RUNNING_TIME,
   ITEM_TEMPERATURE_TOTAL_TIME,
   ITEM_TEMPERATURE_REMAINING_TIME,
   ITEM_TEMPERATURE_RUNNING_TIME,
   ITEM_TEMPERATURE_DATETIME,
   ITEM_TEMPERATURE_VALVE_MAX//never delete this
};
/*****************************Heat***********************************/
const uint8 HEAT_ADDRESS_LEN = 7;
const uint8 HEAT_TEMPERATURE_LEN = 3;
const uint8 HEAT_FLOW_LEN = 5;
const uint8 HEAT_HEAT_LEN = 5;
const uint8 HEAT_RUNNING_TIME_LEN = 3;
struct ValveHeatT
{
   uint8 MacAddress[MACADDRESS_LEN];
   uint8 UserID[USERID_LEN];
   uint8 HeatType;
   uint8 HeatAddress[HEAT_ADDRESS_LEN];
   uint8 InputWaterTemperature[HEAT_TEMPERATURE_LEN];
   uint8 OutputWaterTemperature[HEAT_TEMPERATURE_LEN];
   uint8 TotalFlow[HEAT_FLOW_LEN];
   uint8 TotalHeat[HEAT_HEAT_LEN];
   uint8 TotalRunningTime[HEAT_RUNNING_TIME_LEN];
   uint8 DateTime[FORWARDER_DATETIME_LEN];
}__attribute__( ( packed, aligned(1) ) );
enum ValveHeatTypeE
{
   ITEM_HEAT_MAC_ADDRESS,
   ITEM_HEAT_USER_ID,
   ITEM_HEAT_TYPE,
   ITEM_HEAT_ADDRESS,
   ITEM_HEAT_INPUT_WATER_TEMPERATURE,
   ITEM_HEAT_OUTPUT_WATER_TEMPERATURE,
   ITEM_HEAT_TOTAL_FLOW,
   ITEM_HEAT_TOTAL_HEAT,
   ITEM_HEAT_TOTAL_RUNNING_TIME,
   ITEM_HEAT_DATETIME,
   ITEM_HEAT_VALVE_MAX//never delete this
};

union ValveDataT
{
   ValveTemperatureT ValveTemperature;
   ValveHeatT        ValveHeat;
};

//ValveRecordDataT
struct ValveRecordDataT
{
   uint32 LastChargeIndex;
   uint32 LastConsumeIndex;
   uint32 LastTemperatureIndex;
};

struct ValveElemT
{
   bool IsActive;
   bool IsDataMissing;
   ValveDataT ValveData;
   ValveRecordDataT ValveRecord;
};



enum CommandTypeE
{
   COMMAND_A1=0xA1,
   COMMAND_A2=0xA2,
   COMMAND_A3=0xA3,
   COMMAND_MAX//never delete this
};
enum ValveCtrlTypeE
{
   VALVE_CTRL_NULL=0x00,
   VALVE_CTRL_SET_TIME=0x01,
   VALVE_CTRL_GET_TIME=0x02,
   VALVE_CTRL_GET_VALVE_RECORD=0x03,
   VALVE_CTRL_GET_CHARGE_DATA=0x04,
   VALVE_CTRL_GET_CONSUME_DATA=0x05,
   VALVE_CTRL_GET_TEMPERATURE=0x06,
   VALVE_CTRL_SET_HEAT_TIME=0x07,
   VALVE_CTRL_CONFIG=0x0A,
   VALVE_CTRL_GET_RUNNING_TIME_INFO=0x08,
   VALVE_CTRL_SWITCH_VALVE=0x0E,
   VALVE_CTRL_MAX//never delete this
};
const uint32 MAX_A1_RESPONSE_LEN = 1024;
typedef map<uint16, ValveElemT> ValveListT;//uint16:valve address
struct ForwarderElementT
{
   bool IsOffline;
   uint32 A1ResponseStrLen;
   uint8 A1ResponseStr[MAX_A1_RESPONSE_LEN];
   ValveListT ValveList;
};
typedef map<uint32, ForwarderElementT> ForwarderMapT;//uint32:forward address
struct ForwarderInfoT
{
   bool IsOffline;
   uint32 ForwarderID;
   uint32 A1ResponseStrLen;
   uint8 A1ResponseStr[MAX_A1_RESPONSE_LEN];
   uint32 ValveCount;
};
typedef vector<ForwarderInfoT> ForwarderInfoListT;

class CForwarderMonitor:public IThread
{
  public:
    bool Init(uint32 nStartTime, uint32 nInterval);
    void SetCom(CSerialComm* pCom);
    void SetForwarderType(ForwarderTypeE ForwarderType){m_ForwarderType = ForwarderType;};
    static CForwarderMonitor* GetInstance();
  private:
    static CForwarderMonitor* m_Instance;
    CForwarderMonitor():m_ForwarderType(FORWARDER_TYPE_TEMPERATURE)
                        , m_DayNoonTimer(DAY_TYPE)
                        // , m_DayNoonTimer(HOUR_TYPE)//just for test
                        //, m_DayNoonTimer(MINUTE_TYPE)//just for test
                        , m_ForwardInfoDataReady(false)
                        , m_IsNewUserIDFound(false){}
    static CLock m_ForwarderLock;
    ForwarderTypeE m_ForwarderType;

  private:
    virtual uint32 Run();

  private:
    CRepeatTimer m_RepeatTimer;

  public:
    ForwarderTypeE GetForwarderType(){return m_ForwarderType;}
    bool AddForwareder(uint32 ForwarderID);
    bool GetStatus(StatusE& Status);

  //dayly noon task
  private:
    void ClearValveRecord();
    void DayNoonTimerTask();
  private:
    CFixTimer m_DayNoonTimer;

    //get valve count
  private:
    void GetForwarderInfoTask();
  private:
    bool m_ForwardInfoDataReady;
    CTimer m_ForwardInfoTimeOut;

  private:
    void SaveForwarderMacUserIDTask();
  private:
    bool m_IsNewUserIDFound;

  public:
    bool GetForwarderInfo(ForwarderInfoListT& ForwarderInfoList);
  private:
    void SetForwarderInfo();
  private:
    CLock m_ForwarderInfoListLock;
    ForwarderInfoListT m_ForwarderInfoList;

  public:
    bool GetValveList(vector<ValveElemT>& valves);
  private:
    void SyncCachedValveList();
  private:
    CLock m_ValveListLock;
    vector<ValveElemT> m_ValveList; // just for cache valves

  public:
    void ConfigValve(uint8* pConfigStr, uint32 ConfigStrLen, ValveCtrlTypeE ValveCtrl, uint8& ValveConfigOKCount);

    //update valve values in Forwarder thread
  private:
    void ResetForwarderData();//must firstly be called
    void GetValveTime();
    void GetValveTemperature();
    void GetValveRunningTime();
    void GetValveUserID();
    void PrintUserID();//just for test
  private:
    uint8 SendValveCtrlOneByOne(const uint8* pValveCtrl, const uint32 ValveCtrlLen);//return how many valves succeed
    void SendA1();
    bool SendA2(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID);
    bool SendA3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID);
    bool SendA2A3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID);
  private:
    bool SendCommand(uint8* pCommand, uint32 CommandLen);

  public:
    void SendForwarderData();//update fix time data to Portal thread called by the Portal thread
    void QueryUser(uint8 uid[USERID_LEN], uint8 * data, uint16 len);
    void Prepaid(uint8 uid[USERID_LEN], uint8 * data, uint16 len);

  private:
    bool ParseData(const uint8* pReceiveData, uint32 ReceiveDataLen, const uint8* pSendData, uint32 SendDataLen);
    bool ParseA1Ack(const uint8* pReceiveData, uint32 ReceiveDataLen, uint32 SendForwarderID);
    void UpdateItem(uint32 ForwarderID, uint16 ValveID, const uint8* pData, uint32 DataLen, ValveTemperatureTypeE ValveTemperatureType);

  protected:
    CSerialComm* m_pCommController;
    ForwarderMapT m_DraftForwarderMap;
};
#endif
