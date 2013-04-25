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
#include "CINI.h"
#include <fstream>
#include "IValveMonitorFactory.h"

#ifdef DEBUG_PORTAL
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
      DEBUG("fail to create parcket header Pos(%d)!=%d\n", Pos, PACKET_CTRL_POS);
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
   DEBUG("PacketLen=%d\n", PacketLen);
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


CPortal::CPortal(): m_GetGPRSInforTaskActive(false)
                    , m_IsGPRSInfoReady(false)
                    , m_SentLog(NULL)
                    , m_SerialIndex(0)
                    , m_IsRegistered(false)
                    , m_pGPRS(NULL)
                    // , m_FixHourTimer(MINUTE_TYPE)//just for test
                    , m_FixHourTimer(HOUR_TYPE)
{
   memset(m_IMEI, 0, sizeof(m_IMEI));
   if (access("queues/", F_OK) == -1) {
       mkdir("queues", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
   }
   heatQueue = lqopen("queues/heat");
   valveQueue = lqopen("queues/valve");
   chargeQueue = lqopen("queues/charge");
   consumeQueue = lqopen("queues/consume");
}

bool CPortal::Init(uint32 nInterval)
{
   if( (NULL==m_pGPRS) || (false==m_pGPRS->Open()) )
   {
      return false;
   }
   DEBUG("m_IMEI=%s\n", m_IMEI);
   m_FixHourTimer.Start();
   DEBUG("nInterval=%u\n", nInterval);
   m_HeartBeatTimer.Start(nInterval);

   if( false == CreateLogFile() )
   {
      DEBUG("fail to create log\n");
   }
   DEBUG("end\n");
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
      // CForwarderMonitor::GetInstance()->SendValveData();
      SendValveData();
   }
}

uint32 CPortal::Run()
{
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
         DEBUG("Connected-IMEI=%s\n", m_IMEI);
      }
      m_GPRSLock.UnLock();

      if( IsRegistered() )
      {
         SetLight(LIGHT_GPRS, false);//turn off GPRS light

         HeartBeat();

         GPRS_Receive();
         //CheckNewVersion();
      }else
      {
         SetLight(LIGHT_GPRS, true);//turn on GPRS light
         m_GPRSLock.Lock();
         m_pGPRS->Disconnect();
         m_GPRSLock.UnLock();
      }

      OnFixTimer(); // insert fixed time packet into NotSentList
      SendChargeData();
      SendConsumeData();
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
   DEBUG("IsGPRSConnected=%s\n", IsGPRSConnected?"true":"false");
   if( IsGPRSConnected && (false == m_IsRegistered) )
   {
      uint8 PacketData[MAX_GPRS_PACKET_LEN] = {0};
      uint32 Pos = CreateCtrlPacket(PacketData, sizeof(PacketData), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_REGISTER_UP, m_SerialIndex);
      DEBUG("Pos=%d\n", Pos);
      if(Pos < sizeof(PacketData))
      {
         m_IsRegistered = GPRS_Send(PacketData, Pos);
      }
      DEBUG("Register %s\n", m_IsRegistered?"true":"false");
   }
   return m_IsRegistered;
}

void CPortal::InsertGeneralHeatData(uint8 * data, uint32 len) {
    DEBUG("Insert general heat data: ");
    hexdump(data, len);
    heatQueueLock.Lock();
    lqenqueue(heatQueue, data, len);
    heatQueueLock.UnLock();
}

void CPortal::SendGeneralHeatData() {
    heatQueueLock.Lock();
    //DEBUG("PacketCount=%d\n", m_HeatPacketList.size());
    uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
    while (!lqempty(heatQueue)) {
        uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_GENERAL_UP, m_SerialIndex);
        if (Pos > sizeof(SentPacketBuffer)) {
            DEBUG("Buffer NOT enought\n");
            heatQueueLock.UnLock();
            return;
        }

        uint16 NodeCount = 0;
        Pos += sizeof(NodeCount);
        size_t dlen = 0;
        if (!lqfront(heatQueue, SentPacketBuffer + Pos, &dlen)) {
            DEBUG("Unable to fetch first element in heat queue\n");
            heatQueueLock.UnLock();
            return;
        }
        Pos += dlen;
        NodeCount ++;
        /*
          GeneralHeatPacketListT::iterator Iter = m_HeatPacketList.begin();
          for( ;(Iter != m_HeatPacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-GENERAL_HEAT_DATA_PACKET_LEN-sizeof(uint32)-sizeof(uint16))); Iter++) {
          memcpy(SentPacketBuffer+Pos, Iter->HeatData, sizeof(Iter->HeatData));
          Pos += sizeof(Iter->HeatData);
          NodeCount++;
          }
        */
        memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));

        uint32 UTCTime = (uint32)time(NULL);
        memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
        Pos += sizeof(UTCTime);

        uint16 CRC16 = 0;

        uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
        memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));

        CRC16 = GenerateCRC(SentPacketBuffer, Pos);
        memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
        Pos += sizeof(CRC16);
        if (false == GPRS_Send(SentPacketBuffer, Pos)) {
            DEBUG("fail\n");
            heatQueueLock.UnLock();
            return;
            //WriteNotSentData(SentPacketBuffer, Pos);
        }
        lqdequeue(heatQueue, SentPacketBuffer, &dlen);
    }
    heatQueueLock.UnLock();
}

void CPortal::InsertValveData(uint8 * data, uint32 len) {
    DEBUG("Insert valve data: ");
    hexdump(data, len);
    valveQueueLock.Lock();
    lqenqueue(valveQueue, data, len);
    valveQueueLock.UnLock();
}

void CPortal::SendValveData() {
    valveQueueLock.Lock();
    //DEBUG("PacketCount=%d\n", m_ForwarderPacketList.size());
    uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
    while (!lqempty(valveQueue)) {
        uint32 Pos = 0;
        if (VALVE_DATA_TYPE_HEAT == IValveMonitorFactory::GetInstance()->GetValveDataType()) {
            Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_FORWARDER_HEAT_UP, m_SerialIndex);
        } else {
            Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_FORWARDER_TEMPERATURE_UP, m_SerialIndex);
        }
        if (Pos > sizeof(SentPacketBuffer)) {
            DEBUG("Buffer NOT enought\n");
            valveQueueLock.UnLock();
            return;
        }

        uint16 NodeCount = 0;
        Pos += sizeof(NodeCount);
        size_t dlen = 0;
        if (!lqfront(valveQueue, SentPacketBuffer + Pos, &dlen)) {
            DEBUG("Unable to fetch first element in valve queue\n");
            valveQueueLock.UnLock();
            return;
        }
        Pos += dlen;
        NodeCount ++;
        /*
          ForwarderPacketListT::iterator Iter = m_ForwarderPacketList.begin();
          for (;(Iter != m_ForwarderPacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-MAX_FORWARDER_DATA_LEN-sizeof(uint32)-sizeof(uint16))); Iter++) {
          memcpy(SentPacketBuffer+Pos, Iter->ForwarderData, Iter->ForwarderDataLen);
          Pos += Iter->ForwarderDataLen;
          NodeCount++;
          }
        */
        memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));

        uint32 UTCTime = (uint32)time(NULL);
        memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
        Pos += sizeof(UTCTime);

        uint16 CRC16 = 0;

        uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
        memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
        assert(sizeof(PacketLen) == PACKET_LEN_LEN);

        CRC16 = GenerateCRC(SentPacketBuffer, Pos);
        memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
        Pos += sizeof(CRC16);
        if(false == GPRS_Send(SentPacketBuffer, Pos)) {
            DEBUG("fail\n");
            valveQueueLock.UnLock();
            return;
            //WriteNotSentData(SentPacketBuffer, Pos);
        }
        lqdequeue(valveQueue, SentPacketBuffer, &dlen);
    }
    valveQueueLock.UnLock();
}

void CPortal::SendHeartBeat()
{
   uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
   uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_HEART_BEAT_UP, m_SerialIndex);
   if(Pos > sizeof(SentPacketBuffer))
   {
      DEBUG("Buffer NOT enought\n");
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
   DEBUG("DataLen=%d\n", DataLen);
   if(0 == DataLen)
   {
      return false;
   }

   uint16 FunctionCode = 0;
   memcpy( &FunctionCode, &(pData[PACKET_FUNCTION_CODE_POS]), sizeof(FunctionCode) );
   DEBUG("FunctionCode=%d\n", FunctionCode);

   uint32 UTCTime = 0;
   memcpy( &UTCTime, pData+PACKET_DOWN_UTC_POS, sizeof(UTCTime) );
   uint32 DateTime[DATETIME_LEN] = {0};
   if( TimeStamp2TimeStr(UTCTime, DateTime, sizeof(DateTime)/sizeof(DateTime[0])) )
   {
      DEBUG( "ACK time--%04d-%02d-%02d %02d:%02d:%02d\n", DateTime[0], DateTime[1], DateTime[2], DateTime[3], DateTime[4], DateTime[5] );
      const uint32 MAX_TIME_DIFFERENCE = 30*60;//30 minutes
      uint32 CurrentTimeUTC = 0;
      GetLocalTimeStamp(CurrentTimeUTC);
      uint32 Difference = CurrentTimeUTC>UTCTime? CurrentTimeUTC-UTCTime:UTCTime-CurrentTimeUTC;
      if( Difference > MAX_TIME_DIFFERENCE )
      {
         if( 0 == SetDateTime(DateTime, 0) )
         {
            DEBUG("adjust time OK\n");
         }
      }
   }

   return true;
}

bool CPortal::ParseHeartBeatAck(uint8* pData, uint32 DataLen)
{
   DEBUG("Data\n");
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
      DEBUG("Set DateTime=%04d-%02d-%02d %02d:%02d:%02d\n", DateTime[0], DateTime[1], DateTime[2], DateTime[3], DateTime[4], DateTime[5]);
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
   DEBUG("0x%02X\n", FunCode);
   if( (false == m_IsRegistered) && (FUNCTION_CODE_REGISTER_UP != FunCode) )
   {
      DEBUG("NOT Register\n");
      return false;
   }

   if( NULL == pData || 0 == DataLen )
   {
      return false;
   }

   uint16 SentSerialIndex = 0;
   memcpy( &SentSerialIndex, pData+PACKET_SERIAL_POS, sizeof(SentSerialIndex));
   DEBUG("SentSerialIndex=%u\n", SentSerialIndex);
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
            DEBUG("Send error\n");
            continue;
         }

         //receive data
         for(uint32 j = 0; j < MAX_GPRS_RETRY_COUNT; j++)
         {
            uint8 AckBuffer[MAX_GPRS_PACKET_LEN] = {0};
            uint32 AckBufferLen = sizeof(AckBuffer);
            if(COMM_OK == m_pGPRS->ReceiveData(AckBuffer, AckBufferLen, PACKET_HEADER_FLAG, sizeof(PACKET_HEADER_FLAG), PACKET_LEN_POS, GPRS_TIMEOUT))
            {
               DEBUG("receive OK--AckBufferLen=%u\n", AckBufferLen);
               FlashLight(LIGHT_GPRS);
               if(0 == AckBufferLen)
               {
                  continue;
               }
               if( 0 != memcmp( pData+PACKET_IMEI_POS, AckBuffer+PACKET_IMEI_POS, IMEI_LEN) )
               {
                  PrintData(AckBuffer+PACKET_IMEI_POS, PACKET_HEADER_LEN);
                  DEBUG("IMEI NOT match\n");
                  continue;
               }

               uint16 ReceFunCode = 0;
               memcpy(&ReceFunCode, AckBuffer+PACKET_FUNCTION_CODE_POS, sizeof(ReceFunCode));

               uint16 SentFunCode = 0;
               memcpy(&SentFunCode, pData+PACKET_FUNCTION_CODE_POS, sizeof(SentFunCode));
               if( (SentFunCode+1) != ReceFunCode )
               {
                  DEBUG("Sent FunCode(0x%X):get a new FunCode(0x%X)\n", SentFunCode, ReceFunCode);

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
                  DEBUG("Serial index NOT match:SentIndex(%u):ReceIndex(%u)\n", SentSerialIndex, ReceSerialIndex);
                  continue;
               }

               if( ParseData(AckBuffer, AckBufferLen) )
               {
                  Ret = true;
                  break;
               }
            }else
            {
               DEBUG("receive Error--AckBufferLen=%u\n", AckBufferLen);
            }
         }

         if(true == Ret)
         {
            DEBUG("end--SentSerialIndex=%u OK\n", SentSerialIndex);
            break;
         }
         usleep(1000*200);//AT module sends data one UDP packet per time
      }
      if(false == Ret)
      {
         DEBUG("connection is down\n");
         m_pGPRS->Disconnect();
      }
   }
   m_GPRSLock.UnLock();
   IncSerialIndex();
   return Ret;
}

bool CPortal::GetStatus(Status& Status)
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

bool CPortal::GetGPRSStatus(Status& Status)
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
      DEBUG("LogSentData function code=0x%02X should not be resent\n", FunCode);
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

bool CPortal::CopyLogData()
{
    return false;
}

void CPortal::GetGPRSInfoTask()
{
   DEBUG("m_GetGPRSInforTaskActive=%d, m_IsGPRSInfoReady=%d\n", m_GetGPRSInforTaskActive, m_IsGPRSInfoReady);
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
      DEBUG("GPRSInfo NOT ready\n");
      m_IsGPRSInfoReady = false;
   }
   m_GPRSLock.UnLock();
}

void CPortal::GPRS_Receive()
{
   if(false == m_IsRegistered)
   {
      DEBUG("NOT Register\n");
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
             //bool Ret = false;

            uint16 FunCode = 0;
            memcpy( &FunCode, RecePacketBuffer+PACKET_FUNCTION_CODE_POS, sizeof(FunCode) );
            DEBUG("FunCode=0x%04X\n", FunCode);

            PacketDataT TempPacketData;
            memset( &TempPacketData, 0, sizeof(TempPacketData) );
            memcpy( TempPacketData.PacketData, RecePacketBuffer, ReceBufferLen);
            PacketMapT::value_type TempPacketElement(FunCode, TempPacketData);
            m_ReceivePacketMap.insert(TempPacketElement);
         }else
         {
            DEBUG("IMEI NOT match\n");
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
               DEBUG("NO FunCode(0x%04X) data\n", FunList[i]);
               continue;
            }

            uint16 PacketLen = 0;
            memcpy( &PacketLen, PacketIter->second.PacketData+PACKET_LEN_POS, sizeof(PacketLen) );
            switch( FunList[i] )
            {
               case FUNCTION_CODE_SWITCH_VALVE_DOWN:
                  ReturnData[0] = IValveMonitorFactory::GetInstance()->ConfigValve(VALVE_SWITCH_VALVE,PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS);
                  ReturnDataLen = 1;
                  break;

               case FUNCTION_CODE_VALVE_SET_HEAT_TIME_DOWN:
                  ReturnData[0] = IValveMonitorFactory::GetInstance()->ConfigValve(VALVE_SET_HEAT_TIME, PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS);
                  ReturnDataLen = 1;
                  break;

               case FUNCTION_CODE_VALVE_CONFIG_DOWN:
                  ReturnData[0] = IValveMonitorFactory::GetInstance()->ConfigValve(VALVE_CONFIG, PacketIter->second.PacketData+PACKET_CTRL_POS, PacketLen-PACKET_CTRL_POS);
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
                     DEBUG("OK\n");
                     break;
                  }
               }
            }
         }
      }
   }
   m_GPRSLock.UnLock();
}

void CPortal::InsertChargeData(uint8 * data, uint32 len) {
    DEBUG("Insert charge data: ");
    hexdump(data, len);
    chargeQueueLock.Lock();
    lqenqueue(chargeQueue, data, len);
    chargeQueueLock.UnLock();
}

void CPortal::SendChargeData() {
    chargeQueueLock.Lock();
    // DEBUG("PacketCount = %d\n", m_ChargePacketList.size());
    uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
    while (!lqempty(chargeQueue)) {
        uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_CHARGE_DATA_UP, m_SerialIndex);
        if(Pos > sizeof(SentPacketBuffer)) {
            DEBUG("Buffer NOT enought\n");
            chargeQueueLock.UnLock();
            return;
        }

        uint16 NodeCount = 0;
        Pos += sizeof(NodeCount);
        size_t dlen = 0;
        if (!lqfront(chargeQueue, SentPacketBuffer + Pos, &dlen)) {
            DEBUG("Unable to fetch first element in charge queue\n");
            chargeQueueLock.UnLock();
            return;
        }
        Pos += dlen;
        NodeCount ++;
        /*
        list<ChargePacketT>::iterator Iter = m_ChargePacketList.begin();
        for ( ;(Iter != m_ChargePacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-FORWARDER_CHARGE_PACKET_LEN-sizeof(uint32)-sizeof(uint16))); Iter++) {
            memcpy(SentPacketBuffer+Pos, Iter->ChargePacket, FORWARDER_CHARGE_PACKET_LEN);
            Pos += FORWARDER_CHARGE_PACKET_LEN;
            NodeCount++;
        }
        */
        memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));
        assert(sizeof(NodeCount) == PACKET_SUB_PACKET_COUNT_LEN);

        uint32 UTCTime = (uint32)time(NULL);
        memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
        Pos += sizeof(UTCTime);

        uint16 CRC16 = 0;

        uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
        memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
        assert(sizeof(PacketLen) == PACKET_LEN_LEN);

        CRC16 = GenerateCRC(SentPacketBuffer, Pos);
        memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
        Pos += sizeof(CRC16);
        if(false == GPRS_Send(SentPacketBuffer, Pos)) {
            DEBUG("fail\n");
            chargeQueueLock.UnLock();
            return;
            //WriteNotSentData(SentPacketBuffer, Pos);
        }
        lqdequeue(chargeQueue, SentPacketBuffer, &dlen);
    }
    chargeQueueLock.UnLock();
}

void CPortal::InsertConsumeData(uint8 * data, uint32 len) {
    DEBUG("Insert consume data: ");
    hexdump(data, len);
    consumeQueueLock.Lock();
    lqenqueue(consumeQueue, data, len);
    consumeQueueLock.UnLock();
}

void CPortal::SendConsumeData() {
    consumeQueueLock.Lock();
    // DEBUG("PacketCount=%d\n", m_ConsumePacketList.size());
    uint8 SentPacketBuffer[MAX_GPRS_PACKET_LEN] = {0};
    while (!lqempty(consumeQueue)) {
        uint32 Pos = CreatePacketHeader(SentPacketBuffer, sizeof(SentPacketBuffer), m_IMEI, sizeof(m_IMEI), FUNCTION_CODE_CONSUME_DATA_UP, m_SerialIndex);
        if (Pos > sizeof(SentPacketBuffer)) {
            DEBUG("Buffer NOT enought\n");
            consumeQueueLock.UnLock();
            return;
        }

        uint16 NodeCount = 0;
        Pos += sizeof(NodeCount);
        size_t dlen = 0;
        if (!lqfront(consumeQueue, SentPacketBuffer + Pos, &dlen)) {
            DEBUG("Unable to fetch first element in consume queue\n");
            consumeQueueLock.UnLock();
            return;
        }
        Pos += dlen;
        NodeCount ++;
        /*
        list<ConsumePacketT>::iterator Iter = m_ConsumePacketList.begin();
        for( ;(Iter != m_ConsumePacketList.end()) && (Pos < (MAX_GPRS_PACKET_LEN-FORWARDER_CONSUME_PACKET_LEN-sizeof(uint32)-sizeof(uint16))); Iter++) {
            memcpy(SentPacketBuffer+Pos, Iter->ConsumePacket, FORWARDER_CONSUME_PACKET_LEN);
            Pos += FORWARDER_CONSUME_PACKET_LEN;
            NodeCount++;
        }
        */
        memcpy(SentPacketBuffer+PACKET_SUBPACKET_COUNT_POS, &NodeCount, sizeof(NodeCount));
        assert(sizeof(NodeCount) == PACKET_SUB_PACKET_COUNT_LEN);

        uint32 UTCTime = (uint32)time(NULL);
        memcpy(SentPacketBuffer+Pos, &UTCTime, sizeof(UTCTime));
        Pos += sizeof(UTCTime);

        uint16 CRC16 = 0;

        uint16 PacketLen = Pos - sizeof(PACKET_HEADER_FLAG);
        memcpy(SentPacketBuffer+PACKET_LEN_POS, &(PacketLen), sizeof(PacketLen));
        assert(sizeof(PacketLen) == PACKET_LEN_LEN);

        CRC16 = GenerateCRC(SentPacketBuffer, Pos);
        memcpy(SentPacketBuffer+Pos, &CRC16, sizeof(CRC16));
        Pos += sizeof(CRC16);
        if (false == GPRS_Send(SentPacketBuffer, Pos)) {
            DEBUG("fail\n");
            consumeQueueLock.UnLock();
            return;
            //WriteNotSentData(SentPacketBuffer, Pos);
        }
        lqdequeue(consumeQueue, SentPacketBuffer, &dlen);
    }
    consumeQueueLock.UnLock();
}

#define SECKEY "CONCENTRATOR"
#define PKGKEY "PACKAGE"
void CPortal::CheckNewVersion() {
    if (access("./download", F_OK) == -1) {
        if (mkdir("./download", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
            DEBUG("Cannot make download dir\n");
            system("rm -f /dev/shm/concentrator_000.rar");
            return; // error
        }
    }
    uint8 tmpbuf[16384] = {0};
    uint32 len = 10240;
    if (access("/dev/shm/concentrator_000.rar", F_OK) == -1) {
        m_GPRSLock.Lock();
        DEBUG("Download concentrator_000.rar\n");
        if (COMM_OK == m_pGPRS->HttpGet("http://www.atzgb.com/pdf/concentrator/concentrator_000.rar", tmpbuf, &len)) {
            // dos2unix
            for (uint32 i = 0; i < len; i ++) {
                if (tmpbuf[i] == '\r')
                    tmpbuf[i] = '\n';
            }
            ofstream fout("/dev/shm/concentrator_000.rar");
            fout.write((char *)tmpbuf, len);
            fout.close();
            m_GPRSLock.UnLock();
        } else {
            m_GPRSLock.UnLock();
            DEBUG("Download concentrator_000.rar failed.\n");
            system("rm -f /dev/shm/concentrator_000.rar");
            return;
        }
    }
    DEBUG("Parsing concentrator_000.rar\n");
    CINI ini("/dev/shm/concentrator_000.rar");
    string crc = ini.GetValueString(SECKEY, "CRC", "");
    char buf[64] = {0};
    bzero(buf, 64);
    sprintf(buf, "%08X", CRC32File("./concentrator"));
    if (crc.compare(string(buf)) != 0) {
        // need to upgrade
        int count = ini.GetValueInt(SECKEY, "PACKAGE_COUNT", 0);
        DEBUG("Download %d packages\n", count);
        m_GPRSLock.Lock();
        // check and download slices
        for (int i = 1; i <= count; i ++) {
            char key[32] = {0};
            bzero(key, 32);
            bzero(buf, 64);
            sprintf(key, "concentrator_%03d.rar", i);
            sprintf(buf, "./download/%s", key);
            if (access(buf, F_OK) == 0) {
                char tmp[64] = {0};
                bzero(tmp, 64);
                sprintf(tmp, "%08X", CRC32File(buf));
                string blockCRC = ini.GetValueString(PKGKEY, key, "");
                if (strcmp(tmp, blockCRC.c_str()) == 0) {
                    continue;
                }
            }
            uint8 signalIntesity = 0;
            if (m_pGPRS->GetSignalIntesity(signalIntesity)) {
                if (signalIntesity < 15) {
                    DEBUG("Signal intesity is too weak, ignore %s\n", key);
                    sleep(1);
                    continue;
                }
            } else {
                DEBUG("Cannot get signal intesity, ignore %s\n", key);
                sleep(1);
                continue;
            }
            char url[1024] = {0};
            bzero(url, 1024);
            sprintf(url, "http://www.atzgb.com/pdf/concentrator/%s", key);
            len = 10240;
            if (COMM_OK != m_pGPRS->HttpGet(url, tmpbuf, &len)) {
                // error
                m_GPRSLock.UnLock();
                DEBUG("Download %s fail.\n", url);
                system("rm -f /dev/shm/concentrator_000.rar");
                return;
            }
            ofstream fout(buf, ios::binary);
            fout.write((char *)tmpbuf, len);
            fout.close();
            DEBUG("Download %s success.\n", url);
        }
        m_GPRSLock.UnLock();
        // combine slices

        ofstream fout("/dev/shm/concentrator", ios::binary);
        for (int i = 1; i <= count; i ++) {
            bzero(buf, 64);
            sprintf(buf, "./download/concentrator_%03d.rar", i);
            ifstream fin(buf);
            bzero(tmpbuf, 16384);
            do {
                fin.read((char *)tmpbuf, 16384);
                fout.write((char *)tmpbuf, fin.gcount());
            } while (!fin.eof());
            fin.close();
        }
        fout.close();

        char buf[64] = {0};
        bzero(buf, 64);
        sprintf(buf, "%08X", CRC32File("/dev/shm/concentrator"));
        if (crc.compare(buf) == 0) {
            DEBUG("Upgrade success.\n");
            // okay, upgrade it
            system("chmod 755 /dev/shm/concentrator");
            system("mv /dev/shm/concentrator ./concentrator");
            system("reboot");
        } else {
            DEBUG("CRC is incorrect. Upgrade fail.\n");
        }
    } else {
        DEBUG("It's up to date!\n");
    }
    system("rm -f /dev/shm/concentrator_000.rar");
    return;
}

#undef SECKEY
#undef PKGKEY
