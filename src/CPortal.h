#ifndef __CPORTAL_H
#define __CPORTAL_H
#include<list>
#include<map>
#include"IThread.h"
#include"CSerialComm.h"
#include"CForwarderMonitor.h"
#include"CHeatMonitor.h"
#include"CGprs.h"
#include"CTimer.h"
#include "logqueue.h"
#include <string.h>

using namespace std;

enum FunctionCodeE
{
   FUNCTION_CODE_REGISTER_UP = 0x01,
   FUNCTION_CODE_REGISTER_DOWN = 0x02,
   FUNCTION_CODE_HEART_BEAT_UP = 0x03,
   FUNCTION_CODE_HEART_BEAT_DOWN = 0x04,
   FUNCTION_CODE_FORWARDER_HEAT_UP = 0x05,
   FUNCTION_CODE_FORWARDER_HEAT_DOWN = 0x06,
   FUNCTION_CODE_FORWARDER_TEMPERATURE_UP = 0x07,
   FUNCTION_CODE_FORWARDER_TEMPERATURE_DOWN = 0x08,
   FUNCTION_CODE_GENERAL_UP = 0x09,
   FUNCTION_CODE_GENERAL_DOWN = 0x0A,
   FUNCTION_CODE_CONSUME_DATA_UP = 0x0B,
   FUNCTION_CODE_CONSUME_DATA_DOWN = 0x0C,
   FUNCTION_CODE_CHARGE_DATA_UP = 0x0D,
   FUNCTION_CODE_CHARGE_DATA_DOWN = 0x0E,

   FUNCTION_CODE_SWITCH_VALVE_DOWN = 0x21,
   FUNCTION_CODE_SWITCH_VALVE_UP = 0x22,
   FUNCTION_CODE_VALVE_SET_HEAT_TIME_DOWN = 0x27,
   FUNCTION_CODE_VALVE_SET_HEAT_TIME_UP = 0x28,
   FUNCTION_CODE_VALVE_CONFIG_DOWN = 0x29,
   FUNCTION_CODE_VALVE_CONFIG_UP = 0x30,
   FUNCTION_CODE_MAX//never delete this
};


enum
{
   ITEM_HEAT_DATA,
   ITEM_HEAT_ALARM,
   ITEM_FORWARDER_DATA,
   ITEM_FORWARDER_ALARM,
   ITEM_FORWARDER_CARD_DATA,
   ITEM_DATA_MAX//never delete this
};

struct PacketDataT
{
   uint8 PacketData[MAX_GPRS_PACKET_LEN];
};
typedef map<uint16, PacketDataT> PacketMapT;
typedef list<PacketDataT> PacketListT;
/*******************Forwarder**************************************/
const uint32 MAX_FORWARDER_DATA_LEN = FORWARDER_TYPE_HEAT_DATA_LEN>FORWARDER_TYPE_TEMPERATURE_DATA_LEN? FORWARDER_TYPE_HEAT_DATA_LEN:FORWARDER_TYPE_TEMPERATURE_DATA_LEN;
/******************************************************************/

class CPortal:public IThread
{

public:
    void SetJZQID(uint8 * id) {bzero(this->m_IMEI, IMEI_LEN); memcpy(this->m_IMEI, id, IMEI_LEN);};
   public:
      void SetGPRS(CGprs* pGPRS);
      bool Init(uint32 nInterval);
      void Start();
      static CPortal* GetInstance();
   private:
      static CPortal* m_Instance;
      CPortal();
      ~CPortal() {
          lqclose(heatQueue);
          lqclose(valveQueue);
          lqclose(chargeQueue);
          lqclose(consumeQueue);
      }
      static CLock m_PortalLock;

   public:
      bool GetStatus(Status& Status);
      bool GetGPRSStatus(Status& Status);
      bool GetGPRSConnected(bool& IsConnected);
      bool GetGPRSSignalIntesity(uint8& nSignalIntesity);
      void InsertGeneralHeatData(uint8 * data, uint32 len);
      void InsertValveData(uint8 * data, uint32 len);
      void InsertChargeData(uint8 * data, uint32 len);
      void InsertConsumeData(uint8 * data, uint32 len);

      bool timeReady;

   private:
      void GetGPRSInfoTask();
      void CheckNewVersion();

      uint8 m_nSignalIntesity;
      bool m_GetGPRSInforTaskActive;
      bool m_IsGPRSInfoReady;
      CTimer m_GPRSInfoTimer;

      void SendGeneralHeatData();
      void SendValveData();
      void SendChargeData();
      void SendConsumeData();
      LOGQUEUE * heatQueue;
      LOGQUEUE * valveQueue;
      LOGQUEUE * chargeQueue;
      LOGQUEUE * consumeQueue;
      CLock heatQueueLock;
      CLock valveQueueLock;
      CLock chargeQueueLock;
      CLock consumeQueueLock;

   public:
      bool CopyLogData();
   private:
      bool CreateLogFile();
      void LogSentData(const uint8* pData, uint32 DataLen);
   private:
      CLock m_SentLogLock;
      FILE* m_SentLog;

   private:
      uint16 m_SerialIndex;
      void IncSerialIndex();

   private://register
      bool IsRegistered();
      bool m_IsRegistered;
      //Send Heart beat
   private:
      void SendHeartBeat();

   protected:
      CGprs* m_pGPRS;
      CLock m_GPRSLock;
      uint8 m_IMEI[IMEI_LEN];
   private:
      void OnFixTimer();
   private:
      CFixTimer m_FixHourTimer;
   private:
      void HeartBeat();
   private:
      CRepeatTimer m_HeartBeatTimer;

   private:
      virtual uint32 Run();
   private:
      bool GPRS_Send(uint8* pData, uint32 DataLen);

   private:
      bool ParseData(uint8* pData, uint32 DataLen);
      bool ParseHeartBeatAck(uint8* pData, uint32 DataLen);

   private:
      void GPRS_Receive();
      PacketMapT m_ReceivePacketMap;
      bool SwitchValve(uint8* pData, uint32 DataLen);
      bool SetValveHeatTime(uint8* pData, uint32 DataLen, uint8& ValveConfigOKCount);
      bool ConfigValve(uint8* pData, uint32 DataLen, uint8& ValveConfigOKCount);
};
#endif
