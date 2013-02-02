#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB 1
#endif
#ifndef CFG_DATETIM_LIB
#define CFG_DATETIM_LIB 1
#endif
#include"libs_emsys_odm.h"

#include<unistd.h>
#include<assert.h>
#include"CPortal.h"
#include"Utils.h"

#ifdef DEBUG_PORTAL
#define DEBUG printf
#else
#define DEBUG(...)
#endif

const uint32 MAX_GPRS_RETRY_COUNT = 3;
const int32 GPRS_TIMEOUT = 10*1000*1000;//10 seconds

const uint32 MAX_GET_GPRSINFO_RETRY_COUNT = 3;
const uint32 GPRS_INFO_TIMEOUT = 300;//s


const uint32 PORTAL_TIME_OUT = 1*1000*1000;
const uint16 MAX_SERIAL_INDEX = 19999;

const uint8 PACKET_HEADER_FLAG[] = {0xF0, 0xF0};
const char* SENT_DATA_FILE_NAME = "log.dat";
const uint32 MAX_NOT_SENT_PACKET_COUNT = 100;
/****************************Common********************************************/
const uint32 PACKET_LEN_LEN = sizeof(uint16);
const uint32 PACKET_FUNCTION_CODE_LEN = sizeof(uint16);
const uint32 PACKET_SERIAL_INDEX_LEN = sizeof(uint16);
const uint32 PACKET_SUB_PACKET_COUNT_LEN = sizeof(uint16);
const uint32 PACKET_HEADER_LEN = sizeof(PACKET_HEADER_FLAG)
                                 +PACKET_LEN_LEN
                                 +IMEI_LEN
                                 +PACKET_FUNCTION_CODE_LEN
                                 +PACKET_SERIAL_INDEX_LEN;
const uint32 PACKET_UTC_LEN = sizeof(uint32);
const uint32 PACKET_CRC_LEN = sizeof(uint16);


const uint32 PACKET_LEN_POS = sizeof(PACKET_HEADER_FLAG);
const uint32 PACKET_IMEI_POS = PACKET_LEN_POS+PACKET_LEN_LEN;
const uint32 PACKET_FUNCTION_CODE_POS = PACKET_IMEI_POS+IMEI_LEN;
const uint32 PACKET_SERIAL_POS = PACKET_FUNCTION_CODE_POS+PACKET_FUNCTION_CODE_LEN;
const uint32 PACKET_CTRL_POS = PACKET_SERIAL_POS+PACKET_SERIAL_INDEX_LEN;

//down packet
const uint32 PACKET_DOWN_UTC_POS = 23;

const uint32 PACKET_SUBPACKET_COUNT_POS = sizeof(PACKET_HEADER_FLAG)
                                         +PACKET_LEN_LEN
                                         +IMEI_LEN
                                         +PACKET_FUNCTION_CODE_LEN
                                         +PACKET_SERIAL_INDEX_LEN;
/*********************************Utils function*********************************************/
static uint32 CreatePacketHeader(uint8* pData, uint32 DataLen, uint8* pIMEI, uint32 IMEILen, uint16 FunCode, uint16 SerialIndex)
{
   if( (NULL==pData) || (DataLen<PACKET_HEADER_LEN) || (NULL==pIMEI) || (IMEI_LEN!=IMEILen) )
   {
      return -1;
   }

   memset(pData, 0, DataLen);
   uint32 Pos = 0;
   memcpy(pData+Pos, PACKET_HEADER_FLAG, sizeof(PACKET_HEADER_FLAG));
   Pos += sizeof(PACKET_HEADER_FLAG);

   Pos += PACKET_LEN_LEN;

   memcpy(pData+Pos, pIMEI, IMEILen);
   Pos += IMEILen;

   const uint16 FunctionCode = FunCode;
   memcpy(pData+Pos, &FunctionCode, sizeof(FunctionCode));
   Pos += sizeof(FunctionCode);

   memcpy(pData+Pos, &SerialIndex, sizeof(SerialIndex));
   Pos += sizeof(SerialIndex);

   assert(Pos == PACKET_HEADER_LEN);

   return Pos;
}

static uint32 CreateCtrlPacket(uint8* pPacketBuffer, uint32 pPacketBufferLen, uint8* pIMEI, uint32 IMEILen, uint16 FunCode, uint16 SerialIndex, uint8* pUserData=NULL, uint32 UserDataLen=0)
{
   if( pPacketBufferLen < (PACKET_CTRL_POS+UserDataLen+PACKET_UTC_LEN+PACKET_CRC_LEN) )
   {
      return -1;
   }

   uint32 Pos = CreatePacketHeader(pPacketBuffer, pPacketBufferLen, pIMEI, IMEILen, FunCode, SerialIndex);
   if(PACKET_CTRL_POS != Pos)
   {
      DEBUG("CreateCtrlPacket()-----fail to create parcket header Pos(%d)!=%d\n", Pos, PACKET_CTRL_POS);
      return -1;
   }

   if( pUserData && (UserDataLen > 0) )
   {
      memcpy(pPacketBuffer+Pos, pUserData, UserDataLen);
      Pos += UserDataLen;
   }

   uint32 UTCTime = 0;
   if( false == GetLocalTimeStamp(UTCTime) )
   {
      if(FUNCTION_CODE_REGISTER_UP != FunCode)
      {
         return -1;
      }
   }
   memcpy(pPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
   Pos += sizeof(UTCTime);

   uint16 PacketLen = Pos+PACKET_CRC_LEN-4;//exclude the header and CRC, 4 bytes
   DEBUG("CreateCtrlPacket()-----PacketLen=%d\n", PacketLen);
   memcpy(pPacketBuffer+PACKET_LEN_POS, &PacketLen, sizeof(PacketLen));

   uint16 CRC16 = GenerateCRC(pPacketBuffer, Pos);
   memcpy(pPacketBuffer+Pos, &CRC16, sizeof(CRC16));
   Pos += sizeof(CRC16);

   assert(Pos == (PACKET_CTRL_POS+UserDataLen+PACKET_UTC_LEN+PACKET_CRC_LEN));
   if(Pos == (PACKET_CTRL_POS+UserDataLen+PACKET_UTC_LEN+PACKET_CRC_LEN))
   {
      return Pos;
   }else
   {
      DEBUG( "CreateCtrlPacket()--Pos NOT correct--Pos(%d)!=%d\n", Pos, (PACKET_CTRL_POS+UserDataLen+PACKET_UTC_LEN+PACKET_CRC_LEN) );
      return -1;
   }
}
/********************************************************************************************/
CPortal* CPortal::m_Instance = NULL;
CLock CPortal::m_PortalLock;

CPortal* CPortal::GetInstance()
{
   if( NULL == m_Instance )
   {
      m_PortalLock.Lock();
      if( NULL == m_Instance )
      {
         m_Instance = new CPortal();
      }
      m_PortalLock.UnLock();
   }
   return m_Instance;
}


CPortal::CPortal(): m_SerialIndex(0)
                    // , m_FixHourTimer(MINUTE_TYPE)//just for test
                    , m_FixHourTimer(HOUR_TYPE)
                    , m_SentLog(NULL)
                    , m_IsRegistered(false)
                    , m_pGPRS(NULL)
                    , m_GetGPRSInforTaskActive(false)
                    , m_IsGPRSInfoReady(false)
{
   memset(m_IMEI, 0, sizeof(m_IMEI));
}

bool CPortal::Init(uint32 nInterval)
{
   if( (NULL==m_pGPRS) || (false==m_pGPRS->Open()) )
   {
      return false;
   }
   DEBUG("CPortal::Init()----m_IMEI=%s\n", m_IMEI);
   m_FixHourTimer.Start();
   DEBUG("CPortal::Init()----nInterval=%u\n", nInterval);
   m_HeartBeatTimer.Start(nInterval);
   DEBUG("CPortal::Init()--0--end\n");

   if( false == CreateLogFile() )
   {
      DEBUG("CPortal::Init()----fail to create log\n");
   }
   DEBUG("CPortal::Init()----end\n");
   return true;
}

void CPortal::SetGPRS(CGprs* pGPRS)
{
   if( (NULL == pGPRS) || (m_pGPRS == pGPRS) )
   {
      return;
   }

   m_pGPRS = pGPRS;
}

void CPortal::Start()
{
   IThread::Start();
}

void CPortal::HeartBeat()
{
   DEBUG("CPortal::HeartBeat()\n");
   if( m_HeartBeatTimer.Done() )
   {
      SendHeartBeat();
   }
}

void CPortal::OnFixTimer()
{
   DEBUG("CPortal::OnFixTimer()\n");
   if( m_FixHourTimer.Done() )
   {
      CHeatMonitor::GetInstance()->SendHeatData();
      SendGeneralHeatData();
      // CForwarderMonitor::GetInstance()->SendForwarderData();
      SendForwarderData();
   }
}

void CPortal::InsertForwarderChargeData(uint8* pChargePacket, uint32 ChargePacketLen)
{
   if( (NULL==pChargePacket) || (0==ChargePacketLen) )
   {
      return;
   }

   assert( FORWARDER_CHARGE_PACKET_LEN == ChargePacketLen );
   ChargePacketT ChargePacketItem;
   memcpy( ChargePacketItem.ChargePacket, pChargePacket, sizeof(ChargePacketItem.ChargePacket) );
   m_PortalLock.Lock();
   m_ChargePacketList.push_back(ChargePacketItem);
   m_PortalLock.UnLock();
}

void CPortal::SendForwarderChargeData()
{
   m_PortalLock.Lock();
   DEBUG("CPortal::SendForwarderChargeData()----PacketCount=%d\n", m_ChargePacketList.size());
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   while(m_ChargePacketList.size() > 0)
   {
      uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_CHARGE_DATA_UP, m_SerialIndex);
      if(Pos > sizeof(SentPacketBuffer))
      {
         DEBUG("CPortal::SendForwarderChargeData()---Buffer NOT enought\n");
         m_PortalLock.UnLock();
         return;
      }

      uint16 NodeCount = 0;
      Pos += sizeof(NodeCount);
      list<ChargePacketT>::iterator Iter = m_ChargePacketList.begin();
      for( ;(Iter != m_ChargePacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-FORWARDER_CHARGE_PACKET_LEN-sizeof(uint32)/*UTC*/-sizeof(uint16)/*CRC16*/));
           Iter++)
      {
         memcpy(SentPacketBuffer+Pos, Iter->ChargePacket, FORWARDER_CHARGE_PACKET_LEN);
         Pos += FORWARDER_CHARGE_PACKET_LEN;
         NodeCount++;
      }
      memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));
      assert(sizeof(NodeCount) == PACKET_SUB_PACKET_COUNT_LEN);

      uint32 UTCTime = 0;
      time(((time_t*)(&UTCTime)));
      memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
      Pos += sizeof(UTCTime);

      uint16 CRC16 = 0;

      uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
      memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
      assert(sizeof(PacketLen) == PACKET_LEN_LEN);

      CRC16 = GenerateCRC(SentPacketBuffer, Pos);
      memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
      Pos += sizeof(CRC16);
      if(false == GPRS_Send(SentPacketBuffer, Pos))
      {
         DEBUG("SendForwarderChargeData()----fail\n");
         WriteNotSentData(SentPacketBuffer, Pos);
      }
      m_ChargePacketList.erase(m_ChargePacketList.begin(), Iter);
   }
   m_PortalLock.UnLock();
}

uint32 CPortal::Run()
{
    bool fetched = false;
   DEBUG("CPortal::Run()\n");
   m_GPRSLock.Lock();
   if(NULL == m_pGPRS)
   {
      m_GPRSLock.UnLock();
      return 1;
   }
   m_GPRSLock.UnLock();

   while(1)
   {
      m_GPRSLock.Lock();
      if( false == m_pGPRS->IsConnected() )
      {
         SetLight(LIGHT_GPRS, true);//turn on GPRS light
         m_pGPRS->Connect();
         uint32 IMEILen = sizeof(m_IMEI);
         m_pGPRS->GetIMEI(m_IMEI, IMEILen);
         m_IsRegistered = false;
         DEBUG("CPortal::Run()----Connected-IMEI=%s\n", m_IMEI);
      }
      m_GPRSLock.UnLock();

      if( IsRegistered() )
      {
         SetLight(LIGHT_GPRS, false);//turn off GPRS light
         // ReSendNotSentData();
         // HeartBeat();

         // GPRS_Receive();
         if (!fetched) {
             m_GPRSLock.Lock();
             uint8 tmpbuf[16384] = {0};
             uint32 len = 10240;
             printf(" ============ start to http get\n");
             if (COMM_OK == m_pGPRS->HttpGet("http://www.atzgb.com/pdf/concentrator/concentrator_000.rar", tmpbuf, &len)) {
                 fetched = true;
                 printf(" ================== concentrator content ====================== \n");
                 printf("%s\n", (char *)tmpbuf);
             }
             m_GPRSLock.UnLock();
         }
      }else
      {
         SetLight(LIGHT_GPRS, true);//turn on GPRS light
         m_GPRSLock.Lock();
         m_pGPRS->Disconnect();
         m_GPRSLock.UnLock();
      }

      OnFixTimer(); // insert fixed time packet into NotSentList
      SendForwarderChargeData(); //insert packet into NotSentList
      SendForwarderConsumeData(); //insert packet into Notsentlist
      GetGPRSInfoTask();

      sleep(1);
   }

   return 0;
}

bool CPortal::IsRegistered()
{
   DEBUG("CPortal::IsRegistered()\n");
   m_GPRSLock.Lock();
   bool IsGPRSConnected = m_pGPRS->IsConnected();
   m_GPRSLock.UnLock();
   DEBUG("CPortal::IsRegistered()----IsGPRSConnected=%s\n", IsGPRSConnected?"true":"false");
   if( IsGPRSConnected && (false == m_IsRegistered) )
   {
      uint8 PacketData[MAX_GPRS_PACKET_LEN] = {0};
      uint32 Pos = CreateCtrlPacket(PacketData, sizeof(PacketData), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_REGISTER_UP, m_SerialIndex);
      DEBUG("CPortal::IsRegistered()-----Pos=%d\n", Pos);
      if(Pos < sizeof(PacketData))
      {
         m_IsRegistered = GPRS_Send(PacketData, Pos);
      }
      DEBUG("CPortal::IsRegistered()-----Register %s\n", m_IsRegistered?"true":"false");
   }
   return m_IsRegistered;
}

void CPortal::InsertGeneralHeatData(uint8* pHeatData, uint32 HeatDataLen)
{
   if((NULL==pHeatData) || (0==HeatDataLen))
   {
      return;
   }

   assert(HeatDataLen == GENERAL_HEAT_DATA_PACKET_LEN);

   GeneralHeatPacketT HeatPacket;
   memset(&HeatPacket, 0, sizeof(HeatPacket));
   memcpy(HeatPacket.HeatData, pHeatData, HeatDataLen);

   m_PortalLock.Lock();
   m_HeatPacketList.push_back( HeatPacket );
   m_PortalLock.UnLock();
}

void CPortal::SendGeneralHeatData()
{
   m_PortalLock.Lock();
   DEBUG("CPortal::SendGeneralHeatData()---PacketCount=%d\n", m_HeatPacketList.size());
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   while( m_HeatPacketList.size() > 0 )
   {
      uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_GENERAL_UP, m_SerialIndex);
      if(Pos > sizeof(SentPacketBuffer))
      {
         DEBUG("CPortal::SendGeneralHeatData()---Buffer NOT enought\n");
         m_PortalLock.UnLock();
         return;
      }

      uint16 NodeCount = 0;
      Pos += sizeof(NodeCount );
      GeneralHeatPacketListT::iterator Iter = m_HeatPacketList.begin();
      for( ;(Iter != m_HeatPacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-GENERAL_HEAT_DATA_PACKET_LEN-sizeof(uint32)/*UTC*/-sizeof(uint16)/*CRC16*/));
           Iter++)
      {
         memcpy(SentPacketBuffer+Pos, Iter->HeatData, sizeof(Iter->HeatData));
         Pos += sizeof(Iter->HeatData);
         NodeCount++;
      }
      memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));

      uint32 UTCTime = 0;
      time(((time_t*)(&UTCTime)));
      memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
      Pos += sizeof(UTCTime);

      uint16 CRC16 = 0;

      uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
      memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));

      CRC16 = GenerateCRC(SentPacketBuffer, Pos);
      memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
      Pos += sizeof(CRC16);
      if(false == GPRS_Send(SentPacketBuffer, Pos))
      {
         DEBUG("CPortal::SendGeneralHeatData()----fail\n");
         WriteNotSentData(SentPacketBuffer, Pos);
      }
      m_HeatPacketList.erase(m_HeatPacketList.begin(), Iter);
   }
   m_PortalLock.UnLock();
}

void CPortal::InsertForwarderData(uint8* pForwarderData, uint32 ForwarderDataLen)
{
   if( (NULL==pForwarderData) || (0==ForwarderDataLen) )
   {
      return;
   }

   assert( (FORWARDER_TYPE_TEMPERATURE_DATA_LEN == ForwarderDataLen)
           ||(FORWARDER_TYPE_HEAT_DATA_LEN == ForwarderDataLen) );
   ForwarderPacketT ForwarderPacket;
   ForwarderPacket.ForwarderDataLen = ForwarderDataLen;
   memcpy(ForwarderPacket.ForwarderData, pForwarderData, ForwarderDataLen);
   m_PortalLock.Lock();
   m_ForwarderPacketList.push_back(ForwarderPacket);
   m_PortalLock.UnLock();
}

void CPortal::SendForwarderData()
{
   m_PortalLock.Lock();
   DEBUG("CPortal::SendForwarderData()----PacketCount=%d\n", m_ForwarderPacketList.size());
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   while(m_ForwarderPacketList.size() > 0)
   {
      uint32 Pos = 0;
      if(FORWARDER_TYPE_HEAT == CForwarderMonitor::GetInstance()->GetForwarderType())
      {
         Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_FORWARDER_HEAT_UP, m_SerialIndex);
      }else
      {
         Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_FORWARDER_TEMPERATURE_UP, m_SerialIndex);
      }
      if(Pos > sizeof(SentPacketBuffer))
      {
         DEBUG("CPortal::SendForwarderData()---Buffer NOT enought\n");
         m_PortalLock.UnLock();
         return;
      }

      uint16 NodeCount = 0;
      Pos += sizeof(NodeCount);
      ForwarderPacketListT::iterator Iter = m_ForwarderPacketList.begin();
      for( ;(Iter != m_ForwarderPacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-MAX_FORWARDER_DATA_LEN-sizeof(uint32)/*UTC*/-sizeof(uint16)/*CRC16*/));
           Iter++)
      {
         memcpy(SentPacketBuffer+Pos, Iter->ForwarderData, Iter->ForwarderDataLen);
         Pos += Iter->ForwarderDataLen;
         NodeCount++;
      }
      memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));
      assert(sizeof(NodeCount) == PACKET_SUB_PACKET_COUNT_LEN);

      uint32 UTCTime = 0;
      time(((time_t*)(&UTCTime)));
      memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
      Pos += sizeof(UTCTime);

      uint16 CRC16 = 0;

      uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
      memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
      assert(sizeof(PacketLen) == PACKET_LEN_LEN);

      CRC16 = GenerateCRC(SentPacketBuffer, Pos);
      memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
      Pos += sizeof(CRC16);
      if(false == GPRS_Send(SentPacketBuffer, Pos))
      {
         DEBUG("SendForwarderData()----fail\n");
         WriteNotSentData(SentPacketBuffer, Pos);
      }
      m_ForwarderPacketList.erase(m_ForwarderPacketList.begin(), Iter);
   }
   m_PortalLock.UnLock();
}

void CPortal::SendHeartBeat()
{
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_HEART_BEAT_UP, m_SerialIndex);
   if(Pos > sizeof(SentPacketBuffer))
   {
      DEBUG("CPortal::SendHeartBeat()---Buffer NOT enought\n");
      return;
   }

   uint32 UTCTime = 0;
   if( false == GetLocalTimeStamp(UTCTime) )
   {
      return;
   }
   memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
   Pos += sizeof(UTCTime);

   uint16 PacketLen = Pos-sizeof(PACKET_HEADER_FLAG);
   memcpy(SentPacketBuffer+PACKET_LEN_POS, &PacketLen, sizeof(PacketLen));

   uint16 CRC16 = GenerateCRC(SentPacketBuffer, Pos);
   memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
   Pos += sizeof(CRC16);

   GPRS_Send(SentPacketBuffer, Pos);
}

bool CPortal::ParseData(uint8* pData, uint32 DataLen)
{
   DEBUG("CPortal::ParseData()----DataLen=%d\n", DataLen);
   if(0 == DataLen)
   {
      return false;
   }

   uint16 FunctionCode = 0;
   memcpy( &FunctionCode, &(pData[PACKET_FUNCTION_CODE_POS]), sizeof(FunctionCode) );
   DEBUG("CPortal::ParseData()----FunctionCode=%d\n", FunctionCode);

   uint32 UTCTime = 0;
   memcpy( &UTCTime, pData+PACKET_DOWN_UTC_POS, sizeof(UTCTime) );
   uint32 DateTime[DATETIME_LEN] = {0};
   if( TimeStamp2TimeStr(UTCTime, DateTime, sizeof(DateTime)/sizeof(DateTime[0])) )
   {
      DEBUG( "CPortal::ParseData---ACK time--%04d-%02d-%02d %02d:%02d:%02d\n", DateTime[0], DateTime[1], DateTime[2], DateTime[3], DateTime[4], DateTime[5] );
      const uint32 MAX_TIME_DIFFERENCE = 30*60;//30 minutes
      uint32 CurrentTimeUTC = 0;
      GetLocalTimeStamp(CurrentTimeUTC);
      uint32 Difference = CurrentTimeUTC>UTCTime? CurrentTimeUTC-UTCTime:UTCTime-CurrentTimeUTC;
      if( Difference > MAX_TIME_DIFFERENCE )
      {
         if( 0 == SetDateTime(DateTime, 0) )
         {
            DEBUG("CPortal::ParseData---adjust time OK\n");
         }
      }
   }

   return true;
}

bool CPortal::ParseHeartBeatAck(uint8* pData, uint32 DataLen)
{
   DEBUG("CPortal::ParseHeartBeatAck()----Data\n");
   PrintData(pData, DataLen);
   if( (NULL == pData) || (sizeof(uint32) != DataLen) )
   {
      return false;
   }
   uint32 UTCTime = 0;
   memcpy( &UTCTime, pData, sizeof(UTCTime) );
   uint32 DateTime[6] = {0};
   if(false == TimeStamp2TimeStr(UTCTime, DateTime, sizeof(DateTime)/sizeof(DateTime[0])))
   {
      return false;
   }
   if( 0 != SetDateTime(DateTime, 0) )
   {
      DEBUG("CPortal::ParseHeartBeatAck()--OK--Set DateTime=%04d-%02d-%02d %02d:%02d:%02d\n", DateTime[0], DateTime[1], DateTime[2], DateTime[3], DateTime[4], DateTime[5]);
      return false;
   }
   return true;
}

void CPortal::IncSerialIndex()
{
   m_SerialIndex++;
   if(m_SerialIndex > MAX_SERIAL_INDEX)
   {
      m_SerialIndex = 0;
   }
}

bool CPortal::GPRS_Send(uint8* pData, uint32 DataLen)
{
   LogSentData(pData, DataLen);

   uint16 FunCode = 0;
   memcpy( &FunCode, pData+PACKET_FUNCTION_CODE_POS, sizeof(FunCode) );
   DEBUG("CPortal::GPRS_Send()-----0x%02X\n", FunCode);
   if( (false == m_IsRegistered) && (FUNCTION_CODE_REGISTER_UP != FunCode) )
   {
      DEBUG("CPortal::GPRS_Send()--NOT Register\n");
      return false;
   }

   if( NULL == pData || 0 == DataLen )
   {
      return false;
   }

   uint16 SentSerialIndex = 0;
   memcpy( &SentSerialIndex, pData+PACKET_SERIAL_POS, sizeof(SentSerialIndex));
   DEBUG("CPortal::GPRS_Send()--begin--SentSerialIndex=%u\n", SentSerialIndex);
   PrintData(pData, DataLen);
   bool Ret = false;
   m_GPRSLock.Lock();
   if(m_pGPRS)
   {
      for(uint32 i = 0; i < MAX_GPRS_RETRY_COUNT; i++)
      {
         //send data
         FlashLight(LIGHT_GPRS);
         if(COMM_OK != m_pGPRS->SendData(pData, DataLen, GPRS_TIMEOUT))
         {
            DEBUG("CPortal::GPRS_Send()----Send error\n");
            continue;
         }

         //receive data
         for(uint32 j = 0; j < MAX_GPRS_RETRY_COUNT; j++)
         {
            uint8 AckBuffer[MAX_GPRS_PACKET_LEN] = {0};
            uint32 AckBufferLen = sizeof(AckBuffer);
            if(COMM_OK == m_pGPRS->ReceiveData(AckBuffer, AckBufferLen, PACKET_HEADER_FLAG, sizeof(PACKET_HEADER_FLAG), PACKET_LEN_POS, GPRS_TIMEOUT))
            {
               DEBUG("CPortal::GPRS_Send()--receive OK--AckBufferLen=%u\n", AckBufferLen);
               FlashLight(LIGHT_GPRS);
               if(0 == AckBufferLen)
               {
                  continue;
               }
               if( 0 != memcmp( pData+PACKET_IMEI_POS, AckBuffer+PACKET_IMEI_POS, IMEI_LEN) )
               {
                  PrintData(AckBuffer+PACKET_IMEI_POS, PACKET_HEADER_LEN);
                  DEBUG("CPortal::GPRS_Send()----IMEI NOT match\n");
                  continue;
               }

               uint16 ReceFunCode = 0;
               memcpy(&ReceFunCode, AckBuffer+PACKET_FUNCTION_CODE_POS, sizeof(ReceFunCode));

               uint16 SentFunCode = 0;
               memcpy(&SentFunCode, pData+PACKET_FUNCTION_CODE_POS, sizeof(SentFunCode));
               if( (SentFunCode+1) != ReceFunCode )
               {
                  DEBUG("CPortal::GPRS_Send()----Sent FunCode(0x%X):get a new FunCode(0x%X)\n", SentFunCode, ReceFunCode);

                  PacketDataT TempPacketData;
                  memset( &TempPacketData, 0, sizeof(TempPacketData) );
                  memcpy( TempPacketData.PacketData, AckBuffer, AckBufferLen );
                  PacketMapT::value_type TempPacketElement(ReceFunCode, TempPacketData);
                  m_ReceivePacketMap.insert(TempPacketElement);

                  j=0;//get a valid data but NOT wanted means server is live, retry again
                  continue;
               }
               if( 0 != memcmp( pData+PACKET_SERIAL_POS, AckBuffer+PACKET_SERIAL_POS, PACKET_SERIAL_INDEX_LEN ) )
               {
                  uint16 SentSerialIndex = 0;;
                  memcpy(&SentSerialIndex, pData+PACKET_SERIAL_POS, sizeof(SentSerialIndex));
                  uint16 ReceSerialIndex = 0;;
                  memcpy(&ReceSerialIndex, AckBuffer+PACKET_SERIAL_POS, sizeof(ReceSerialIndex));
                  DEBUG("CPortal::GPRS_Send()----Serial index NOT match:SentIndex(%u):ReceIndex(%u)\n", SentSerialIndex, ReceSerialIndex);
                  continue;
               }

               if( ParseData(AckBuffer, AckBufferLen) )
               {
                  Ret = true;
                  break;
               }
            }else
            {
               DEBUG("CPortal::GPRS_Send()--receive Error--AckBufferLen=%u\n", AckBufferLen);
            }
         }

         if(true == Ret)
         {
            DEBUG("CPortal::GPRS_Send()--end--SentSerialIndex=%u OK\n", SentSerialIndex);
            break;
         }
         usleep(1000*200);//AT module sends data one UDP packet per time
      }
      if(false == Ret)
      {
         DEBUG("CPortal::GPRS_Send()----connection is down\n");
         m_pGPRS->Disconnect();
      }
   }
   m_GPRSLock.UnLock();
   IncSerialIndex();
   return Ret;
}

bool CPortal::GetStatus(StatusE& Status)
{
   if( m_PortalLock.TryLock() )
   {
      if( false == IsStarted() )
      {
         Status = STATUS_ERROR;
      }else
      {
         Status = STATUS_OK;
      }
      m_PortalLock.UnLock();
      return true;
   }
   return false;
}

bool CPortal::GetGPRSStatus(StatusE& Status)
{
   if( m_GPRSLock.TryLock() )
   {
      if( m_pGPRS && m_pGPRS->IsConnected() )
      {
         Status = STATUS_OK;
      }else
      {
         Status = STATUS_ERROR;
      }
      m_GPRSLock.UnLock();
      return true;
   }
   return false;
}

bool CPortal::GetGPRSConnected(bool& IsConnected)
{
   bool Ret = false;
   if( m_GPRSLock.TryLock() )
   {
      if(m_pGPRS)
      {
         IsConnected = m_pGPRS->IsConnected();
         Ret = true;
      }
      m_GPRSLock.UnLock();
   }
   return Ret;
}

bool CPortal::GetGPRSSignalIntesity(uint8& nSignalIntesity)
{
   DEBUG("CPortal::GetGPRSSignalIntesity()\n");
   bool Ret = false;
   if( m_GPRSLock.TryLock() )
   {
      if(m_IsGPRSInfoReady)
      {
         nSignalIntesity = m_nSignalIntesity;
         Ret = true;
      }else
      {
         m_GetGPRSInforTaskActive = true;
      }
      m_GPRSLock.UnLock();
   }
   return Ret;
}

bool CPortal::CreateLogFile()
{
   m_SentLogLock.Lock();
   if(m_SentLog)
   {
      fclose(m_SentLog);
      m_SentLog = NULL;
   }
   m_SentLog = fopen(SENT_DATA_FILE_NAME, "a+");
   if(NULL == m_SentLog)
   {
      m_SentLogLock.UnLock();
      return false;
   }
   m_SentLogLock.UnLock();

   return true;
}

void CPortal::LogSentData(const uint8* pData, uint32 DataLen)
{
   DEBUG("CPortal::LogSentData()\n");
   if(NULL == pData || 0 == DataLen || DataLen > MAX_GPRS_PACKET_LEN)
   {
      assert(0);
      return;
   }
   uint16 FunCode = 0;
   memcpy( &FunCode, pData+PACKET_FUNCTION_CODE_POS, sizeof(FunCode) );

   const uint16 FunList[] = { FUNCTION_CODE_FORWARDER_HEAT_UP
                              , FUNCTION_CODE_FORWARDER_TEMPERATURE_UP
                              , FUNCTION_CODE_GENERAL_UP
                              , FUNCTION_CODE_CONSUME_DATA_UP
                              , FUNCTION_CODE_CHARGE_DATA_UP};
   uint32 i = 0;
   for(; i < sizeof(FunList)/sizeof(FunList[0]); i++)
   {
      if(FunList[i] == FunCode)
      {
         break;
      }
   }
   if( i >= sizeof(FunList)/sizeof(FunList[0]) )
   {
      DEBUG("CPortal::LogSentData()-----LogSentData function code=0x%02X should not be resent\n", FunCode);
      return;
   }

   m_SentLogLock.Lock();

   if(NULL == m_SentLog)
   {
      m_SentLog = fopen(SENT_DATA_FILE_NAME, "a+");
   }
   if(NULL == m_SentLog)
   {
      m_SentLogLock.UnLock();
      return;
   }
   PrintData(pData, DataLen);
   fwrite(pData, DataLen, 1, m_SentLog);
   fflush(m_SentLog);

   m_SentLogLock.UnLock();
}

void CPortal::WriteNotSentData(const uint8* pData, uint32 DataLen)
{
   if(NULL == pData || 0 == DataLen || DataLen>MAX_GPRS_PACKET_LEN)
   {
      assert(0);
      return;
   }
   if(m_NotSentPacketList.size() > MAX_NOT_SENT_PACKET_COUNT)
   {
      DEBUG("CPortal::WriteNotSentData()----m_NotSentPacketList is full:MAX_NOT_SENT_PACKET_COUNT=%u\n", MAX_NOT_SENT_PACKET_COUNT);
      return;
   }

   PacketDataT NotSentPacket;
   memset( &NotSentPacket, 0, sizeof(NotSentPacket) );
   memcpy(NotSentPacket.PacketData, pData, DataLen);

   m_NotSentPacketList.push_back(NotSentPacket);
}

void CPortal::ReSendNotSentData()
{
   DEBUG("CPortal::ReSendNotSentData()----NOT sent packet count=%d\n", m_NotSentPacketList.size());
   if(0 == m_NotSentPacketList.size())
   {
      return;
   }
   PacketListT::iterator PacketIter = m_NotSentPacketList.begin();
   uint16 PacketLen = 0;
   memcpy( &PacketLen, PacketIter->PacketData+PACKET_LEN_POS, sizeof(PacketLen) );
   PacketLen += 4;//PacketData NOT include header(2 bytes) and CRC16(2 bytes)
   uint32 SentLen = PacketLen;
   if( GPRS_Send(PacketIter->PacketData, PacketLen) )
   {
      m_NotSentPacketList.erase(PacketIter);
   }
}

bool CPortal::CopyLogData()
{
}

void CPortal::GetGPRSInfoTask()
{
   DEBUG("CCPortal::GetGPRSInfoTask()----m_GetGPRSInforTaskActive=%d, m_IsGPRSInfoReady=%d\n", m_GetGPRSInforTaskActive, m_IsGPRSInfoReady);
   m_GPRSLock.Lock();
   if( m_GetGPRSInforTaskActive )
   {
      if( false == m_IsGPRSInfoReady )
      {
         m_nSignalIntesity = UNKNOWN_SIGNAL_INTESITY;
         for(uint32 i = 0; i < MAX_GET_GPRSINFO_RETRY_COUNT; i++)
         {
            if( m_pGPRS->GetSignalIntesity(m_nSignalIntesity) )
            {
               break;
            }
         }
         m_IsGPRSInfoReady = true;
         m_GPRSInfoTimer.StartTimer(GPRS_INFO_TIMEOUT);
      }

      if( m_IsGPRSInfoReady )
      {
         m_GetGPRSInforTaskActive = false;
      }
   }

   if( m_GPRSInfoTimer.Done() )
   {
      DEBUG("CCPortal::GetGPRSInfoTask()----GPRSInfo NOT ready\n");
      m_IsGPRSInfoReady = false;
   }
   m_GPRSLock.UnLock();
}

void CPortal::GPRS_Receive()
{
   if(false == m_IsRegistered)
   {
      DEBUG("CPortal::GPRS_Receive()--NOT Register\n");
      return;
   }

   DEBUG("CPortal::GPRS_Receive()\n");
   m_GPRSLock.Lock();
   if(m_pGPRS)
   {
      uint8 RecePacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
      uint32 ReceBufferLen = sizeof(RecePacketBuffer);
      if( (COMM_OK == m_pGPRS->ReceiveData(RecePacketBuffer, ReceBufferLen, PACKET_HEADER_FLAG, sizeof(PACKET_HEADER_FLAG), PACKET_LEN_POS, GPRS_TIMEOUT)) && (ReceBufferLen > 0) )
      {
         DEBUG("CPortal::GPRS_Receive()\n");
         FlashLight(LIGHT_GPRS);
         PrintData(RecePacketBuffer, ReceBufferLen);
         if( 0 == memcmp( RecePacketBuffer+PACKET_IMEI_POS, m_IMEI, sizeof(m_IMEI)) )
         {
            bool Ret = false;

            uint16 FunCode = 0;
            memcpy( &FunCode, RecePacketBuffer+PACKET_FUNCTION_CODE_POS, sizeof(FunCode) );
            DEBUG("CPortal::GPRS_Receive()--FunCode=0x%04X\n", FunCode);

            PacketDataT TempPacketData;
            memset( &TempPacketData, 0, sizeof(TempPacketData) );
            memcpy( TempPacketData.PacketData, RecePacketBuffer, ReceBufferLen);
            PacketMapT::value_type TempPacketElement(FunCode, TempPacketData);
            m_ReceivePacketMap.insert(TempPacketElement);
         }else
         {
            DEBUG("CPortal::GPRS_Receive()--IMEI NOT match\n");
         }

         const uint16 FunList[] = { FUNCTION_CODE_SWITCH_VALVE_DOWN, FUNCTION_CODE_VALVE_SET_HEAT_TIME_DOWN, FUNCTION_CODE_VALVE_CONFIG_DOWN };
         for(uint32 i = 0; i < sizeof(FunList)/sizeof(FunList[0]);i += 2)
         {
            const uint32 MAX_RETURN_DATA_LEN = 128;
            uint8 ReturnData[MAX_RETURN_DATA_LEN] = {0};
            uint32 ReturnDataLen = 0;

            PacketMapT::iterator PacketIter = m_ReceivePacketMap.find(FunList[i]);
            if( PacketIter == m_ReceivePacketMap.end() )
            {
               DEBUG("CPortal::GPRS_Receive()--NO FunCode(0x%04X) data\n", FunList[i]);
               continue;
            }

            uint16 PacketLen = 0;
            memcpy( &PacketLen, PacketIter->second.PacketData+PACKET_LEN_POS, sizeof(PacketLen) );
            switch( FunList[i] )
            {
               case FUNCTION_CODE_SWITCH_VALVE_DOWN:
                  CForwarderMonitor::GetInstance()->ConfigValve(PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS, VALVE_CTRL_SWITCH_VALVE, ReturnData[0]);
                  ReturnDataLen = 1;
                  break;

               case FUNCTION_CODE_VALVE_SET_HEAT_TIME_DOWN:
                  CForwarderMonitor::GetInstance()->ConfigValve(PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS, VALVE_CTRL_SET_HEAT_TIME, ReturnData[0]);
                  ReturnDataLen = 1;
                  break;

               case FUNCTION_CODE_VALVE_CONFIG_DOWN:
                  CForwarderMonitor::GetInstance()->ConfigValve(PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS, VALVE_CTRL_CONFIG, ReturnData[0]);
                  ReturnDataLen = 1;
                  break;

               default:
                  break;
            }

            uint16 SerialIndex = 0;
            memcpy( &SerialIndex, PacketIter->second.PacketData+PACKET_SERIAL_POS, sizeof(SerialIndex) );
            uint8 SendPacketData[MAX_GPRS_PACKET_LEN] = {0};
            // uint32 SendPacketLen = CreateCtrlPacket(SendPacketData, sizeof(SendPacketData), m_IMEI, sizeof(m_IMEI), FunList[i+1], SerialIndex, ReturnData, ReturnDataLen);
            uint32 SendPacketLen = CreateCtrlPacket(SendPacketData, sizeof(SendPacketData), m_IMEI, sizeof(m_IMEI), FunList[i]+1, SerialIndex, ReturnData, ReturnDataLen);
            if( SendPacketLen <= sizeof(SendPacketData) )
            {//get a correct packet
               for(uint32 j = 0; j < MAX_GPRS_RETRY_COUNT; j++)
               {
                  if(COMM_OK == m_pGPRS->SendData(SendPacketData, SendPacketLen, GPRS_TIMEOUT))
                  {
                     m_ReceivePacketMap.erase(PacketIter);
                     DEBUG("CPortal::GPRS_Receive()----OK\n");
                     break;
                  }
               }
            }
         }
      }
   }
   m_GPRSLock.UnLock();
}

void CPortal::InsertForwarderConsumeData(uint8* pConsumePacket, uint32 ConsumePacketLen)
{
   if( (NULL == pConsumePacket) || (0 == ConsumePacketLen) )
   {
      return;
   }

   DEBUG("CPortal::InsertForwarderConsumeData():\n");
   PrintData(pConsumePacket, ConsumePacketLen);
   assert( FORWARDER_CONSUME_PACKET_LEN == ConsumePacketLen );
   ConsumePacketT ConsumePacketItem;
   memcpy( ConsumePacketItem.ConsumePacket, pConsumePacket, sizeof(ConsumePacketItem.ConsumePacket) );
   m_PortalLock.Lock();
   m_ConsumePacketList.push_back(ConsumePacketItem);
   m_PortalLock.UnLock();
}

void CPortal::SendForwarderConsumeData()
{
   m_PortalLock.Lock();
   DEBUG("CPortal::SendForwarderConsumeData()----PacketCount=%d\n", m_ConsumePacketList.size());
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   while(m_ConsumePacketList.size() > 0)
   {
      uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_CONSUME_DATA_UP, m_SerialIndex);
      if(Pos > sizeof(SentPacketBuffer))
      {
         DEBUG("CPortal::SendForwarderConsumeData()---Buffer NOT enought\n");
         m_PortalLock.UnLock();
         return;
      }

      uint16 NodeCount = 0;
      Pos += sizeof(NodeCount);
      list<ConsumePacketT>::iterator Iter = m_ConsumePacketList.begin();
      for( ;(Iter != m_ConsumePacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-FORWARDER_CONSUME_PACKET_LEN-sizeof(uint32)/*UTC*/-sizeof(uint16)/*CRC16*/));
           Iter++)
      {
         memcpy(SentPacketBuffer+Pos, Iter->ConsumePacket, FORWARDER_CONSUME_PACKET_LEN);
         Pos += FORWARDER_CONSUME_PACKET_LEN;
         NodeCount++;
      }
      memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));
      assert(sizeof(NodeCount) == PACKET_SUB_PACKET_COUNT_LEN);

      uint32 UTCTime = 0;
      time(((time_t*)(&UTCTime)));
      memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
      Pos += sizeof(UTCTime);

      uint16 CRC16 = 0;

      uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
      memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
      assert(sizeof(PacketLen) == PACKET_LEN_LEN);

      CRC16 = GenerateCRC(SentPacketBuffer, Pos);
      memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
      Pos += sizeof(CRC16);
      if(false == GPRS_Send(SentPacketBuffer, Pos))
      {
         DEBUG("CPortal::SendForwarderConsumeData()----fail\n");
         WriteNotSentData(SentPacketBuffer, Pos);
      }
      m_ConsumePacketList.erase(m_ConsumePacketList.begin(), Iter);
   }
   m_PortalLock.UnLock();
}
