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
/*******************General heat**************************************/
struct GeneralHeatPacketT
{
   uint8 HeatData[GENERAL_HEAT_DATA_PACKET_LEN];
};
typedef list<GeneralHeatPacketT> GeneralHeatPacketListT;
/*********************************************************/
/*******************Forwarder**************************************/
const uint32 MAX_FORWARDER_DATA_LEN = FORWARDER_TYPE_HEAT_DATA_LEN>FORWARDER_TYPE_TEMPERATURE_DATA_LEN? FORWARDER_TYPE_HEAT_DATA_LEN:FORWARDER_TYPE_TEMPERATURE_DATA_LEN;
struct ForwarderPacketT
{
   uint32 ForwarderDataLen;
   uint8  ForwarderData[MAX_FORWARDER_DATA_LEN];
};
typedef list<ForwarderPacketT> ForwarderPacketListT;
/******************************************************************/

class CPortal:public IThread
{
   public:
      void SetGPRS(CGprs* pGPRS);
      bool Init(uint32 nInterval);
      void Start();
      static CPortal* GetInstance();
   private:
      static CPortal* m_Instance;
      CPortal();
      static CLock m_PortalLock;

   public:
      bool GetStatus(StatusE& Status);
      bool GetGPRSStatus(StatusE& Status);
      bool GetGPRSConnected(bool& IsConnected);
      bool GetGPRSSignalIntesity(uint8& nSignalIntesity);
      void InsertGeneralHeatData(uint8* pHeatData, uint32 HeatDataLen);
      void InsertForwarderData(uint8* pForwarderData, uint32 ForwarderDataLen);

   private:
      void GetGPRSInfoTask();
   private:
      uint8 m_nSignalIntesity;
   private:
      bool m_GetGPRSInforTaskActive;
      bool m_IsGPRSInfoReady;
      CTimer m_GPRSInfoTimer;

      //General heat data
   private:
      void SendGeneralHeatData();
      GeneralHeatPacketListT m_HeatPacketList;
      //Forward data
   private:
      void SendForwarderData();
      ForwarderPacketListT m_ForwarderPacketList;

   public:
      bool CopyLogData();
   private:
      bool CreateLogFile();
      void LogSentData(const uint8* pData, uint32 DataLen);
   private:
      CLock m_SentLogLock;
      FILE* m_SentLog;
   private:
      void WriteNotSentData(const uint8* pData, uint32 DataLen);
      void ReSendNotSentData();
   private:
      PacketListT m_NotSentPacketList;

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

   public:
      void InsertForwarderChargeData(uint8* pChargePacket, uint32 ChargePacketLen);
   private:
      void SendForwarderChargeData();
   private:
      struct ChargePacketT
      {
         uint8 ChargePacket[FORWARDER_CHARGE_PACKET_LEN];
      };
      list<ChargePacketT> m_ChargePacketList;

   public:
      void InsertForwarderConsumeData(uint8* pConsumePacket, uint32 ConsumePacketLen);
   private:
      void SendForwarderConsumeData();
   private:
      struct ConsumePacketT
      {
         uint8 ConsumePacket[FORWARDER_CONSUME_PACKET_LEN];
      };
      list<ConsumePacketT> m_ConsumePacketList;

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
