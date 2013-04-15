#include<assert.h>
#include"CForwarderMonitor.h"
#include"CPortal.h"
#include"Utils.h"
#include "CCardHost.h"
#include "logdb.h"

#ifndef CFG_TIMER_LIB
#define CFG_TIMER_LIB
#endif
#include"libs_emsys_odm.h"

#ifdef DEBUG_FORWARDER
#define DEBUG printf
#ifndef hexdump
#define hexdump(data, len) do {for (uint32 i = 0; i < (uint32)len; i ++) { printf("%02x ", data[i]);} printf("\n");} while(0)
#endif
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
#endif


/***************************************************************************/
const uint8 FORWARDER_COMMAND_BEGIN_FLAG[] = {0xAA, 0x80};
const uint8 FORWARDER_COMMAND_END_FLAG[] = {0xEE, 0xDD};
const uint32 FORWARDER_TIMEOUT = 3*1000*1000;//3 seconds
const uint8 FORWARDER_INTERVAL = 3;//3 seconds
const uint32 FORWARDERINFO_TIMEOUT = 300;//300 seconds
const uint32 FORWARDER_A2_A3_INTERVAL = 500*1000*3;//500ms
const uint8 INVALID_USERID[USERID_LEN] = {0};
const uint32 FORWARDER_VALVE_READ_MAX_COUNT_ONE_TIME = 16;
const uint32 MAX_A2_A3_RETRY_COUNT = 2;
/**********************************************************************
 * Valve get userID command, it is a special command*******************
 * its format is not the same as the other*****************************
 * ********************************************************************/
const uint8 VALVE_PACKET_FLAG= 0xF1;
const uint8 VALVE_CTRL_USERID_FLAG = 0xF1;/*0xF1 0xF1*/
/****************************************************/

const uint8 VALVE_CTRL_FLAG = 0x0F;
const uint8 VALVE_CTRL_RANDAM = 0xFB;
const uint8 VALVE_CTRL_LEN_OFFSET = 2;
const uint8 VALVE_CTRL_COMMAND_OFFSET = 3;
/*************************************Utils function******************************************************/
static bool IsFixTimeCommand(const uint8* pValveCtrl, const uint32 ValveCtrlLen)
{
   if( (NULL == pValveCtrl) || (0 == ValveCtrlLen) )
   {
      return false;
   }

   const uint8 FIX_TIME_DATA_COMMAND[] = {VALVE_CTRL_GET_TEMPERATURE, VALVE_CTRL_GET_TIME, VALVE_CTRL_GET_RUNNING_TIME_INFO};
   const uint8 VALVE_CTRL_POS = 3;
   for(uint32 i = 0; i < sizeof(FIX_TIME_DATA_COMMAND)/sizeof(FIX_TIME_DATA_COMMAND[0]); i++)
   {
      if( FIX_TIME_DATA_COMMAND[i] == pValveCtrl[VALVE_CTRL_POS] )
      {
         return true;
      }
   }

   return false;
}
/*******************************************************************************************/

CLock CForwarderMonitor::m_ForwarderLock;
CForwarderMonitor* CForwarderMonitor::m_Instance = NULL;
CForwarderMonitor* CForwarderMonitor::GetInstance()
{
   if( NULL == m_Instance )
   {
      m_ForwarderLock.Lock();
      if( NULL == m_Instance )
      {
         m_Instance = new CForwarderMonitor();
      }
      m_ForwarderLock.UnLock();
   }
   return m_Instance;
}

bool CForwarderMonitor::Init(uint32 nStartTime, uint32 nInterval)
{
   return (m_RepeatTimer.Start(nInterval*60)) && (m_DayNoonTimer.Start(12/*noon*/));
}

void CForwarderMonitor::SetCom(CSerialComm* pCom)
{
   if( (NULL == pCom) || (m_pCommController == pCom) )
   {
      return;
   }

   m_pCommController = pCom;
}

uint32 CForwarderMonitor::Run()
{
    uint32 i = 0;
    LOGDB * db = dbopen((char *)"users", DB_RDONLY, sizeof(user_t));
    if (db != NULL) {
        user_t user;
        size_t r;
        do {
            r = dbseq(db, &user, R_NEXT);
            if (r > 0) {
                users.push_back(user);
            }
        } while (r > 0);
        dbclose(db);
    }
    while(1) {
        DEBUG("CForwarderMonitor::Run()----begin\n");
        ResetForwarderData();

        if (i % 60 == 0 || users.size() == 0) {
            SendA1();
            // PrintUserID();

            GetValveUserID();

            // PrintUserID();
            if (m_IsNewUserIDFound)
                SaveForwarderMacUserIDTask();
        }
        if( m_RepeatTimer.Done() ) {
            m_ForwarderLock.Lock();

            GetValveTemperature();
            // PrintUserID();
            GetValveTime();
            // PrintUserID();
            GetValveRunningTime();
            // PrintUserID();

            m_ForwarderLock.UnLock();


            SendForwarderData();
            // PrintUserID();
        }

        SendCardHostCommand();

        GetForwarderInfoTask();
        // PrintUserID();

        DayNoonTimerTask();
        // PrintUserID();

        DEBUG("CForwarderMonitor::Run()---end\n");
        sleep(1);
        i ++;
    }
}

void CForwarderMonitor::SendA1()
{
   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ )
   {
      uint8 QueryCommand[MAX_PACKET_LEN] = {0};//query
      uint32 Pos = 0;
      DEBUG("CForwarderMonitor::SendA1()----Query forwarder\n");
      QueryCommand[FORWARDER_COMMAND_ID_POS] = COMMAND_A1;
      Pos = FORWARDER_ID_POS;
      memcpy(QueryCommand+Pos, &(ForwarderIter->first), FORWARDER_ID_LEN);
      Pos += FORWARDER_ID_LEN;
      if( SendCommand(QueryCommand, Pos) )
      {
         ForwarderIter->second.IsOffline = false;
      }else
      {
         ForwarderIter->second.IsOffline = true;
         DEBUG("CForwarderMonitor::SendA1()-----Can't get valve address\n");
      }
   }

   //Forwarder Valve data is ready
   m_ForwardInfoDataReady = true;
   m_ForwardInfoTimeOut.StartTimer(FORWARDERINFO_TIMEOUT);
   SetForwarderInfo();
}

bool CForwarderMonitor::SendA2(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
   if(NULL == pValveCtrl || 0 == ValveCtrlLen)
   {
      assert(0);
      return false;
   }

   uint8 A2Command[MAX_PACKET_LEN] = {0};//query
   //send read command to each forwarder
   A2Command[FORWARDER_COMMAND_ID_POS] = COMMAND_A2;
   uint32 Pos = FORWARDER_ID_POS;
   memcpy(A2Command+Pos, &ForwarderID, FORWARDER_ID_LEN);
   Pos += FORWARDER_ID_LEN;
   A2Command[Pos] = 0x01;//Valve count
   Pos++;
   memcpy(A2Command+Pos, &ValveID, VALVE_ID_LEN);
   Pos += VALVE_ID_LEN;
   memcpy(A2Command+Pos, pValveCtrl, ValveCtrlLen);
   Pos += ValveCtrlLen;
   if( SendCommand(A2Command, Pos) )
   {
      return true;
   }else
   {
      return false;
   }
}

bool CForwarderMonitor::SendA3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
   if(NULL == pValveCtrl || 0 == ValveCtrlLen)
   {
      assert(0);
      return false;
   }

   DEBUG("CForwarderMonitor::SendA3()\n");

   uint8 A3Command[MAX_PACKET_LEN] = {0};//query
   A3Command[FORWARDER_COMMAND_ID_POS] = COMMAND_A3;
   uint32 Pos = FORWARDER_ID_POS;
   memcpy(A3Command+Pos, &ForwarderID, FORWARDER_ID_LEN);
   Pos += FORWARDER_ID_LEN;
   assert(Pos == A3_VALVE_ID_POS);
   memcpy(A3Command+Pos, &ValveID, VALVE_ID_LEN);
   Pos += VALVE_ID_LEN;
   if( SendCommand(A3Command, Pos) )
   {
      return true;
   }else
   {
      return false;
   }
}

bool CForwarderMonitor::SendA2A3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
   for(uint32 i = 0; i < MAX_A2_A3_RETRY_COUNT; i++)
   {
      if( SendA2(pValveCtrl, ValveCtrlLen, ForwarderID, ValveID) )
      {
         sleep(FORWARDER_INTERVAL);
         if( SendA3(pValveCtrl, ValveCtrlLen, ForwarderID, ValveID) )
         {
            DEBUG("CForwarderMonitor::SendA2A3()-----ForwarderID(0x%8X) ValveID(%04X) OK\n", ForwarderID, ValveID);
            return true;
         }
      }
   }
   DEBUG("CForwarderMonitor::SendA2A3()-----ForwarderID(0x%8X) ValveID(%04X) Error\n", ForwarderID, ValveID);
   return false;
}

void CForwarderMonitor::ResetForwarderData()//must firstly be called in each loop
{
   DEBUG("CForwarderMonitor::ResetForwarderData()\n");
   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++)
   {
      for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
      {
         ValveIter->second.IsActive = false;
         ValveIter->second.IsDataMissing = false;
      }
   }
}

void CForwarderMonitor::GetValveTime()
{
   DEBUG("CForwarderMonitor::GetValveTime()\n");
   const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_CTRL_GET_TIME};
   SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::GetValveTemperature()
{
   DEBUG("CForwarderMonitor::GetValveTemperature()\n");
   //get Valve Record
   if( false == m_ForwardInfoDataReady )
   {
      SendA1();
   }

   ClearValveRecord();
   const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_CTRL_GET_VALVE_RECORD};
   SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));

   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ )
   {
      for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
      {
         if( false == ValveIter->second.IsActive )
         {
            DEBUG("CForwarderMonitor::GetValveTemperature()----ForwarderID=0x%08X, ValveID=0x%04X NOT active\n", ForwarderIter->first, ValveIter->first);
            continue;
         }

         if( ValveIter->second.IsDataMissing )
         {
            DEBUG("CForwarderMonitor::GetValveTemperature()----ForwarderID=0x%08X, ValveID=0x%04X Data missing\n", ForwarderIter->first, ValveIter->first);
            continue;
         }

         if(-1 == (int32)ValveIter->second.ValveRecord.LastTemperatureIndex)
         {
            ValveIter->second.IsDataMissing = true;
            DEBUG("CForwarderMonitor::GetValveTemperature()----ForwarderID=0x%08X, ValveID=0x%04X LastTemperatureIndex=-1\n", ForwarderIter->first, ValveIter->first);
            continue;
         }

         const uint32 VALVE_CTRL_GET_TEMPERATURE_RECORD_INDEX_POS = 4;
         uint8 GetValveConsumeDataCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x03, VALVE_CTRL_GET_TEMPERATURE, 0xFF, 0xFF/*a invalid index*/};
         GetValveConsumeDataCommand[VALVE_CTRL_GET_TEMPERATURE_RECORD_INDEX_POS] = ValveIter->second.ValveRecord.LastTemperatureIndex/256;
         GetValveConsumeDataCommand[VALVE_CTRL_GET_TEMPERATURE_RECORD_INDEX_POS+1] = ValveIter->second.ValveRecord.LastTemperatureIndex%256;

         if( false == SendA2A3(GetValveConsumeDataCommand, sizeof(GetValveConsumeDataCommand), ForwarderIter->first, ValveIter->first) )
         {
            DEBUG("CForwarderMonitor::GetValveTemperature()--fail--ForwarderID=0x%08X, ValveID=0x%04X\n", ForwarderIter->first, ValveIter->first);
            ValveIter->second.IsDataMissing = true;
         }
      }
   }
}

void CForwarderMonitor::GetValveRunningTime()
{
   DEBUG("CForwarderMonitor::GetValveRunningTime()\n");
   const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_CTRL_GET_RUNNING_TIME_INFO};
   SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::GetValveUserID()
{
   DEBUG("CForwarderMonitor::GetValveUserID()\n");
   const uint8 ValveCommand[] = {VALVE_CTRL_USERID_FLAG, VALVE_CTRL_USERID_FLAG, 0x01};
   SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::DayNoonTimerTask()
{
   if( m_DayNoonTimer.Done() )
   {
      DEBUG("CForwarderMonitor::DayNoonTimerTask()\n");
      m_ForwarderLock.Lock();

      //get Valve Record
      if( false == m_ForwardInfoDataReady )
      {
         SendA1();
      }

      ClearValveRecord();
      const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_CTRL_GET_VALVE_RECORD};
      SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));

      for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ )
      {
         if( ForwarderIter->second.IsOffline )
         {
            DEBUG("CForwarderMonitor::DayNoonTimerTask()-----0x%8x offline\n", ForwarderIter->first);
            continue;
         }
         for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
         {
            if(false == ValveIter->second.IsActive)
            {
               DEBUG("CForwarderMonitor::DayNoonTimerTask()-----ForwarderID(0x%8X) ValveID(%04X) offline\n", ForwarderIter->first, ValveIter->first);
               continue;
            }

            //Get consume data
            if(-1 != (int32)ValveIter->second.ValveRecord.LastConsumeIndex)
            {
               const uint32 VALVE_CTRL_GET_CONSUME_RECORD_INDEX_POS = 4;
               uint8 GetValveConsumeDataCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x03, VALVE_CTRL_GET_CONSUME_DATA, 0xFF, 0xFF/*a invalid index*/};
               GetValveConsumeDataCommand[VALVE_CTRL_GET_CONSUME_RECORD_INDEX_POS] = ValveIter->second.ValveRecord.LastConsumeIndex/256;
               GetValveConsumeDataCommand[VALVE_CTRL_GET_CONSUME_RECORD_INDEX_POS+1] = ValveIter->second.ValveRecord.LastConsumeIndex%256;
               SendA2A3(GetValveConsumeDataCommand, sizeof(GetValveConsumeDataCommand), ForwarderIter->first, ValveIter->first);
            }

            //Get charge data
            if(-1 != (int32)ValveIter->second.ValveRecord.LastChargeIndex)
            {
               const uint32 VALVE_CTRL_GET_CHARGE_RECORD_INDEX_POS = 4;
               uint8 GetValveChargeDataCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x02, VALVE_CTRL_GET_CHARGE_DATA, 0xFF/*a invalid index*/};
               GetValveChargeDataCommand[VALVE_CTRL_GET_CHARGE_RECORD_INDEX_POS] = ValveIter->second.ValveRecord.LastChargeIndex;
               SendA2A3(GetValveChargeDataCommand, sizeof(GetValveChargeDataCommand), ForwarderIter->first, ValveIter->first);
            }
         }
      }

      m_ForwarderLock.UnLock();
   }
}

void CForwarderMonitor::ClearValveRecord()
{
   DEBUG("CForwarderMonitor::ClearValveRecord()\n");
   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ )
   {
      for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
      {
         ValveIter->second.ValveRecord.LastChargeIndex = -1;
         ValveIter->second.ValveRecord.LastConsumeIndex = -1;
         ValveIter->second.ValveRecord.LastTemperatureIndex = -1;
      }
   }
}

bool CForwarderMonitor::SendCommand(uint8* pCommand, uint32 CommandLen)
{
   if(NULL == m_pCommController)
   {
      DEBUG("CForwarderMonitor::SendCommand()--COMM is NULL\n");
      return false;
   }

   uint8 SendCommand[MAX_PACKET_LEN] = {0};
   memcpy(SendCommand, FORWARDER_COMMAND_BEGIN_FLAG, sizeof(FORWARDER_COMMAND_BEGIN_FLAG));
   uint32 Pos = sizeof(FORWARDER_COMMAND_BEGIN_FLAG);
   memcpy(SendCommand+Pos, pCommand, CommandLen);
   Pos += CommandLen;
   memcpy(SendCommand+Pos, FORWARDER_COMMAND_END_FLAG, sizeof(FORWARDER_COMMAND_END_FLAG));
   Pos += sizeof(FORWARDER_COMMAND_END_FLAG);
   SendCommand[Pos] = GenerateXorParity(SendCommand, Pos);
   Pos++;


   for(uint8 i=0; i<MAX_RETRY_COUNT;i++)
   {
      DEBUG("CForwarderMonitor::SendCommand()--Send Command--Try %d\n", i);
      hexdump(SendCommand, Pos);
      FlashLight(LIGHT_FORWARD);
      if( COMM_OK != m_pCommController->WriteBuf(SendCommand, Pos, FORWARDER_TIMEOUT) )
      {
         DEBUG("WriteError\n");
         continue;
      }

      uint8 Buffer[MAX_BUFFER_LEN] = {0};
      uint32 BufferCount = sizeof(Buffer);
      FlashLight(LIGHT_FORWARD);
      if(COMM_OK == m_pCommController->ReadBuf(Buffer
                                             , BufferCount
                                             , FORWARDER_COMMAND_BEGIN_FLAG
                                             , sizeof(FORWARDER_COMMAND_BEGIN_FLAG)
                                             , FORWARDER_COMMAND_END_FLAG
                                             , sizeof(FORWARDER_COMMAND_END_FLAG)
                                             , FORWARDER_TIMEOUT) )
      {
         if(ParseData(Buffer+sizeof(FORWARDER_COMMAND_BEGIN_FLAG)
                      , BufferCount-sizeof(FORWARDER_COMMAND_BEGIN_FLAG)-sizeof(FORWARDER_COMMAND_END_FLAG)
                      , pCommand
                      , CommandLen))
         {
            return true;
         }else
         {
            sleep(1);
         }
      }
   }
   return false;
}

bool CForwarderMonitor::AddForwareder(uint32 ForwarderID)
{
   if( m_DraftForwarderMap.end() != m_DraftForwarderMap.find(ForwarderID) )
   {
      return false;
   }
   DEBUG("CForwarderMonitor::AddForwareder(ForwarderID=0x%08x)\n", ForwarderID);
   memset( &(m_DraftForwarderMap[ForwarderID]), 0, sizeof(m_DraftForwarderMap[ForwarderID]) );
   m_DraftForwarderMap[ForwarderID].IsOffline = true;
   m_DraftForwarderMap[ForwarderID].ValveList.clear();
   return true;
}

bool CForwarderMonitor::ParseA1Ack(const uint8* pReceiveData, uint32 ReceiveDataLen, uint32 SendForwarderID)
{
   if( NULL == pReceiveData || ReceiveDataLen <= 0 )
   {
      return false;
   }

   uint32 ReceiveForwarderID = 0;
   memcpy(&ReceiveForwarderID, pReceiveData+FORWARDER_ID_POS, sizeof(ReceiveForwarderID));
   assert(ReceiveForwarderID == SendForwarderID);
   uint32 Pos = A1_ACK_COMPOSITE_VALVE_COUNT_POS;
   uint8 CompositeValveCount = pReceiveData[Pos];
   Pos += VALVE_COUNT_LEN;

   DEBUG("CForwarderMonitor::ParseA1Ack(SendForwarderID=0x%08x)----CompositeValveCount=0x%02x\n", SendForwarderID, CompositeValveCount);
   for(uint8 CompositeValveIndex = 0; (CompositeValveIndex<CompositeValveCount) && (Pos<ReceiveDataLen); CompositeValveIndex++)
   {
      uint16 ValveID = 0;
      memcpy(&(ValveID), pReceiveData+Pos, VALVE_ID_LEN);
      if( m_DraftForwarderMap[SendForwarderID].ValveList.end() == m_DraftForwarderMap[SendForwarderID].ValveList.find(ValveID) )
      {//it is a new valve
         DEBUG("CForwarderMonitor::ParseA1Ack()--New valve-1-ReceiveForwarderID=0x%08x, ValveID=0x%04x\n", ReceiveForwarderID, ValveID);
         memset( &(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]), 0, sizeof(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]) );
      }
      m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].IsActive = true;
      memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress, &ReceiveForwarderID, 2);
      memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress+2, &ValveID, VALVE_ID_LEN);
      DEBUG("CForwarderMonitor::ParseA1Ack()--1--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                                                  , ReceiveForwarderID
                                                  , ValveID
                                                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                                                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                                                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                                                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
      Pos += VALVE_ID_LEN;
      uint8 ChildValveCount = pReceiveData[Pos];
      DEBUG("CForwarderMonitor::ParseA1Ack()----ChildValveCount=0x%02x\n", ChildValveCount);
      Pos += VALVE_COUNT_LEN;
      for(uint8 ChildValveIndex = 0; ChildValveIndex < ChildValveCount; ChildValveIndex++)
      {
         memcpy(&(ValveID), pReceiveData+Pos, VALVE_ID_LEN);
         if( m_DraftForwarderMap[SendForwarderID].ValveList.end() == m_DraftForwarderMap[SendForwarderID].ValveList.find(ValveID) )
         {//it is a new valve
            DEBUG("CForwarderMonitor::ParseA1Ack()--New valve-2-ReceiveForwarderID=0x%08x, ValveID=0x%04x\n", ReceiveForwarderID, ValveID);
            memset( &(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]), 0, sizeof(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]) );
         }
         m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].IsActive = true;
         memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress, &ReceiveForwarderID, 2);
         memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress+2, &ValveID, VALVE_ID_LEN);
         DEBUG("CForwarderMonitor::ParseA1Ack()--2--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                                                     , ReceiveForwarderID
                                                     , ValveID
                                                     , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                                                     , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                                                     , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                                                     , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
         Pos += VALVE_ID_LEN;
      }
   }
   if(Pos != ReceiveDataLen)
   {
      DEBUG("CForwarderMonitor::ParseA1Ack()----Pos=%d, ReceiveDataLen=%d\n", Pos, ReceiveDataLen);
      assert(0);
      return false;
   }

   if( ReceiveDataLen < sizeof(m_DraftForwarderMap[SendForwarderID].A1ResponseStr) )
   {
      m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen = 0;
      //frame header
      memcpy(m_DraftForwarderMap[SendForwarderID].A1ResponseStr+m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen, FORWARDER_COMMAND_BEGIN_FLAG, sizeof(FORWARDER_COMMAND_BEGIN_FLAG));
      m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen += sizeof(FORWARDER_COMMAND_BEGIN_FLAG);
      //frame content
      memcpy(m_DraftForwarderMap[SendForwarderID].A1ResponseStr+m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen, pReceiveData, ReceiveDataLen);
      m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen += ReceiveDataLen;
      //frame end
      memcpy(m_DraftForwarderMap[SendForwarderID].A1ResponseStr+m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen, FORWARDER_COMMAND_END_FLAG, sizeof(FORWARDER_COMMAND_END_FLAG));
      m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen += sizeof(FORWARDER_COMMAND_END_FLAG);
      //get frame crc
      uint8 CRC = GenerateXorParity(m_DraftForwarderMap[SendForwarderID].A1ResponseStr, m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen);
      memcpy(m_DraftForwarderMap[SendForwarderID].A1ResponseStr+m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen, &CRC, sizeof(CRC));
      m_DraftForwarderMap[SendForwarderID].A1ResponseStrLen += sizeof(CRC);
   }

   return true;
}

bool CForwarderMonitor::ParseData(const uint8* pReceiveData, uint32 ReceiveDataLen, const uint8* pSendData, uint32 SendDataLen)
{
   if( NULL == pReceiveData || ReceiveDataLen <= 0 || NULL == pSendData || SendDataLen <= 0 )
   {
      return false;
   }

   DEBUG("CForwarderMonitor::ParseData()---SendData----");
   PrintData(pSendData, SendDataLen);
   DEBUG("CForwarderMonitor::ParseData()---ReceiveData----");
   PrintData(pReceiveData, ReceiveDataLen);
   if(pReceiveData[FORWARDER_COMMAND_ID_POS] != pSendData[FORWARDER_COMMAND_ID_POS])
   {
      DEBUG("CForwarderMonitor::ParseData()----Command NOT match Send(0x%02x)--Receive(0x%02x)\n", pSendData[FORWARDER_COMMAND_ID_POS], pReceiveData[FORWARDER_COMMAND_ID_POS]);
      assert(0);
      return false;
   }

   switch( pReceiveData[FORWARDER_COMMAND_ID_POS] )
   {
      case COMMAND_A1:
         {
            uint32 SendForwarderID = 0;
            memcpy(&SendForwarderID, pSendData+FORWARDER_ID_POS, FORWARDER_ID_LEN);
            return ParseA1Ack(pReceiveData , ReceiveDataLen , SendForwarderID);
         }
         break;

      case COMMAND_A2:
         {
            assert(FORWARDER_COMMAND_ID_LEN+FORWARDER_ID_LEN  == ReceiveDataLen);
            if(FORWARDER_COMMAND_ID_LEN+FORWARDER_ID_LEN == ReceiveDataLen)
            {
               return true;
            }else
            {
               DEBUG("CForwarderMonitor::ParseData()----AckLen NOT correct Needed(%d)==ActualLen(%d)\n", FORWARDER_ID_LEN, ReceiveDataLen);
               PrintData(pReceiveData, ReceiveDataLen);
               assert(0);
               return false;
            }
         }

      case COMMAND_A3:
         {
            if(VALVE_CTRL_NULL == pReceiveData[A3_VALVE_CTRL_POS])
            {
               DEBUG("CForwarderMonitor::ParseData()----No Valve data\n");
               return false;
            }

            uint32 ForwarderID = 0;
            memcpy(&ForwarderID, pReceiveData+FORWARDER_ID_POS, FORWARDER_ID_LEN);
            uint16 ValveID = 0;
            memcpy(&ValveID, pReceiveData+A3_VALVE_ID_POS, VALVE_ID_LEN );
            uint32 CalcLen = A3_VALVE_CTRL_POS+pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]+VALVE_CTRL_COMMAND_OFFSET;
            if( VALVE_CTRL_USERID_FLAG == pReceiveData[A3_VALVE_CTRL_POS])
            {//get userID valve command
               CalcLen = A3_VALVE_CTRL_POS+3+pReceiveData[A3_VALVE_CTRL_POS+2];
               assert(pReceiveData[A3_VALVE_CTRL_POS+2] == USERID_LEN);
               if( ReceiveDataLen != CalcLen )
               {
                  DEBUG("CForwarderMonitor::ParseData()--get UserID--ForwarderID=0x%08X, ValveID=0x%04X, Len NOT match\n", ForwarderID, ValveID);
                  return false;
               }
               UpdateItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_CTRL_POS+3, USERID_LEN, ITEM_TEMPERATURE_USER_ID);
               m_IsNewUserIDFound = true;
               return true;
            }

            uint8 A3CtrlRet = pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET];
            DEBUG("CForwarderMonitor::ParseData()----A3RetPos=%u, A3CtrlRet=0x%02X\n", A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET, A3CtrlRet);
            if( A3CtrlRet&0xE0 )//the high 3 bits = 0 when A2 command is exec successfully
            {
               DEBUG("CForwarderMonitor::ParseData()----A3CtrlRet=0x%02X\n A2 command fails\n", A3CtrlRet);
               return false;
            }

            DEBUG("CForwarderMonitor::ParseData()----A3_VALVE_CTRL_POS=%u, Flag=0x%02X\n", A3_VALVE_CTRL_POS, pReceiveData[A3_VALVE_CTRL_POS]);
            switch(pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET])
            {
               case VALVE_CTRL_GET_TIME:
                  {
                     PrintData( pReceiveData+A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]);

                     uint16 Year = ((pReceiveData[A3_VALVE_VALUE_POS]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS]&0x0F);
                     Year = Year*100 + ((pReceiveData[A3_VALVE_VALUE_POS+1]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+1]&0x0F);
                     uint8 Month = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)]&0x0F);
                     uint8 Day = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)]&0x0F);
                     uint8 Hour = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)]&0x0F);
                     uint8 Minute = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)]&0x0F);
                     uint8 Second = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)+sizeof(Minute)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)+sizeof(Minute)]&0x0F);

                     uint32 ValveUTCTime = DateTime2TimeStamp(Year, Month, Day, Hour, Minute, Second);
                     UpdateItem(ForwarderID, ValveID, (uint8*)&ValveUTCTime, sizeof(ValveUTCTime), ITEM_TEMPERATURE_DATETIME);

                     return true;
                  }

               case VALVE_CTRL_GET_TEMPERATURE:
                  {
                     const uint32 INDOOR_TEMPERATURE_OFFSET = 5;
                     UpdateItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_INDOOR_TEMPERATURE);
                     UpdateItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS+FORWARDER_TEMPERATURE_LEN, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_SET_TEMPERATURE);
                     UpdateItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS+FORWARDER_TEMPERATURE_LEN+FORWARDER_TEMPERATURE_LEN, 1, ITEM_TEMPERATURE_VALVE_RUNNING_TIME);
                     return true;
                  }

               case VALVE_CTRL_GET_RUNNING_TIME_INFO:
                  UpdateItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_TOTAL_TIME);
                  UpdateItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS+FORWARDER_TIME_LEN, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_REMAINING_TIME);
                  UpdateItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS+FORWARDER_TIME_LEN+FORWARDER_TIME_LEN, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_RUNNING_TIME);
                  return true;

               case VALVE_CTRL_SWITCH_VALVE:
                  DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_SWITCH_VALVE\n");
                  return true;

               case VALVE_CTRL_SET_HEAT_TIME:
                  DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_SET_HEAT_TIME\n");
                  return true;

               case VALVE_CTRL_CONFIG:
                  DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_CONFIG\n");
                  return true;

               case VALVE_CTRL_GET_VALVE_RECORD:
                  {
                     DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_GET_VALVE_RECORD\n");
                     //charge data
                     const uint8 CHARGE_RECORD_INDEX_POS = 0;
                     m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastChargeIndex = pReceiveData[A3_VALVE_VALUE_POS+CHARGE_RECORD_INDEX_POS]-1;
                     //consume data
                     const uint8 CONSUME_RECORD_INDEX_POS = 1;
                     m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastConsumeIndex = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_RECORD_INDEX_POS]*256 + pReceiveData[A3_VALVE_VALUE_POS+CONSUME_RECORD_INDEX_POS+1] - 1;
                     //Temperature data
                     const uint8 TEMPERATURE_RECORD_INDEX_POS = 3;
                     m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastTemperatureIndex = pReceiveData[A3_VALVE_VALUE_POS+TEMPERATURE_RECORD_INDEX_POS]*256 + pReceiveData[A3_VALVE_VALUE_POS+TEMPERATURE_RECORD_INDEX_POS+1] - 1;
                     return true;
                  }

               case VALVE_CTRL_GET_CHARGE_DATA:
                  {
                     DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_GET_CHARGE_DATA\n");
                     uint8 ChargePacketDataLen = FORWARDER_CHARGE_PACKET_LEN-2;
                     uint8 ChargePacketData[FORWARDER_CHARGE_PACKET_LEN] = {0};
                     uint32 Pos = 0;
                     ChargePacketData[0] = VALVE_PACKET_FLAG;
                     Pos++;
                     ChargePacketData[1] = ChargePacketDataLen;
                     Pos++;
                     ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.find(ForwarderID);
                     if( (ForwarderIter != m_DraftForwarderMap.end()) && (ForwarderIter->second.ValveList.end() != ForwarderIter->second.ValveList.find(ValveID)) )
                     {
                        if(0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID)))
                        {
                           DEBUG("CForwarderMonitor::ParseData()--VALVE_CTRL_GET_CHARGE_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
                           return false;
                        }
                        memcpy( ChargePacketData+Pos
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID
                              , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID) );
                        Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID);

                        memcpy( ChargePacketData+Pos
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress
                              , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress) );
                        Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress);

                        memcpy(ChargePacketData+Pos, pReceiveData+A3_VALVE_VALUE_POS, FORWARDER_CHARGE_PACKET_LEN-Pos);
                        CPortal::GetInstance()->InsertForwarderChargeData(ChargePacketData, sizeof(ChargePacketData));
                     }
                     return true;
                  }

               case VALVE_CTRL_GET_CONSUME_DATA:
                  {
                     DEBUG("CForwarderMonitor::ParseData()--OK--VALVE_CTRL_GET_CONSUME_DATA\n");
                     uint8 ConsumePacketDataLen = FORWARDER_CONSUME_PACKET_LEN-2;
                     uint8 ConsumePacketData[FORWARDER_CONSUME_PACKET_LEN] = {0};
                     uint32 Pos = 0;
                     ConsumePacketData[0] = VALVE_PACKET_FLAG;
                     Pos++;
                     ConsumePacketData[1] = ConsumePacketDataLen;
                     Pos++;
                     ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.find(ForwarderID);
                     if( (ForwarderIter != m_DraftForwarderMap.end()) && (ForwarderIter->second.ValveList.end() != ForwarderIter->second.ValveList.find(ValveID)) )
                     {
                        if(0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID)))
                        {
                           DEBUG("CForwarderMonitor::ParseData()--VALVE_CTRL_GET_CONSUME_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                                 , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
                           return false;
                        }

                        memcpy( ConsumePacketData+Pos
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress
                              , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress) );
                        Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress);

                        memcpy( ConsumePacketData+Pos
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID
                              , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID) );
                        Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID);

                        const uint8 CONSUME_DATA_OFFSET = 4;
                        const uint8 CONSUME_DATA_LENGTH = 4;
                        memcpy(ConsumePacketData+Pos, pReceiveData+A3_VALVE_VALUE_POS+CONSUME_DATA_OFFSET, CONSUME_DATA_LENGTH);
                        const uint8 PACKET_CONSUME_DATA_LENGTH = 24;
                        Pos += PACKET_CONSUME_DATA_LENGTH;

                        const uint8 CONSUME_DATA_DATETIME_OFFSET = 0;
                        //const uint8 CONSUME_DATA_DATETIME_LENGTH = 4;
                        uint8 DateTimeStr[7] = {0};
                        DateTimeStr[6] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET];//yy
                        DateTimeStr[5] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+1];//yy
                        DateTimeStr[4] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+2];//mm
                        DateTimeStr[3] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+3];//dd
                        uint32 DateTime = DateTime2TimeStamp(DateTimeStr, sizeof(DateTimeStr));
                        memcpy(ConsumePacketData+Pos, &DateTime, sizeof(DateTime));
                        CPortal::GetInstance()->InsertForwarderConsumeData(ConsumePacketData, sizeof(ConsumePacketData));
                     }
                     return true;
                  }
            case VALVE_CTRL_QUERY_USER:
                // it's query user command response
                CCardHost::GetInstance()->AckQueryUser((uint8 *)pReceiveData + A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]);
                return true;
            case VALVE_CTRL_PREPAID:
                // it's query user command response
                CCardHost::GetInstance()->AckPrepaid((uint8 *)pReceiveData + A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]);
                return true;
            }
         }
         break;
   }
   return false;
}

uint8 CForwarderMonitor::SendValveCtrlOneByOne(const uint8* pValveCtrl, const uint32 ValveCtrlLen)
{
   if(NULL == pValveCtrl || ValveCtrlLen <= 0)
   {
      DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----Parameter error\n");
      return 0;
   }
   uint8 ValveAckCount = 0;

   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ )
   {
      if( ForwarderIter->second.IsOffline )
      {
         DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----0x%8x offline\n", ForwarderIter->first);
         continue;
      }
      for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
      {
         if(false == ValveIter->second.IsActive)
         {
            DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) offline\n", ForwarderIter->first, ValveIter->first);
            continue;
         }

         if( IsFixTimeCommand(pValveCtrl, ValveCtrlLen) )
         {
            if(ValveIter->second.IsDataMissing)
            {
               DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) data missing\n", ForwarderIter->first, ValveIter->first);
               continue;
            }
         }

         const uint8 GetUserIDCommand[] = {VALVE_CTRL_USERID_FLAG, VALVE_CTRL_USERID_FLAG, 0x01};
         if( (ValveCtrlLen == sizeof(GetUserIDCommand)) && (0 == memcmp(GetUserIDCommand, pValveCtrl, sizeof(GetUserIDCommand))) )
         {
            DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----UserID\n");
            if( memcmp(ValveIter->second.ValveData.ValveTemperature.UserID, INVALID_USERID, sizeof(INVALID_USERID)) )
            {
               DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) UserID already exists, no need to reget\n", ForwarderIter->first, ValveIter->first);
               continue;
            }
         }

         if( SendA2A3(pValveCtrl, ValveCtrlLen, ForwarderIter->first, ValveIter->first) )
         {
            DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) OK\n", ForwarderIter->first, ValveIter->first);
            ValveAckCount++;
         }else
         {
            if( IsFixTimeCommand(pValveCtrl, ValveCtrlLen) )
            {
               DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) Data missing\n", ForwarderIter->first, ValveIter->first);
               ValveIter->second.IsDataMissing = true;
            }
            if( (ValveCtrlLen == sizeof(GetUserIDCommand)) && (0 == memcmp(GetUserIDCommand, pValveCtrl, sizeof(GetUserIDCommand))) )
            {
               DEBUG("CForwarderMonitor::SendValveCtrlOneByOne()-----ForwarderID(0x%8X) ValveID(%04X) UserID missing\n", ForwarderIter->first, ValveIter->first);
               ValveIter->second.IsDataMissing = true;
            }
         }
      }
   }

   return ValveAckCount;
}

void CForwarderMonitor::UpdateItem(uint32 ForwarderID, uint16 ValveID, const uint8* pData, uint32 DataLen, ValveTemperatureTypeE ValveTemperatureType)
{
   if( NULL == pData || 0 >= DataLen )
   {
      return;
   }
   DEBUG("CForwarderMonitor::UpdateItem()--Temperature(%d)--ForwarderID=0x%08x, ValveID=0x%04x\n", ValveTemperatureType, ForwarderID, ValveID);
   PrintData(pData, DataLen);

   switch(ValveTemperatureType)
   {
      case ITEM_TEMPERATURE_USER_ID:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID));
         DEBUG("CForwarderMonitor::UpdateItem()--UserID--ForwarderID=0x%08x, ValveID=0x%04x\n", ForwarderID, ValveID);
         PrintData(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID));
         break;

      case ITEM_TEMPERATURE_INDOOR_TEMPERATURE:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.IndoorTemperature)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.IndoorTemperature, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.IndoorTemperature));
         break;

      case ITEM_TEMPERATURE_SET_TEMPERATURE:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.SetTemperature)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.SetTemperature, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.SetTemperature));
         break;

      case ITEM_TEMPERATURE_VALVE_RUNNING_TIME:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveRunningTime)== DataLen);
         memcpy(&(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveRunningTime), pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveRunningTime));
         break;

      case ITEM_TEMPERATURE_TOTAL_TIME:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveTotalTime)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveTotalTime, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.ValveTotalTime));
         break;

      case ITEM_TEMPERATURE_REMAINING_TIME:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RemainingTime)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RemainingTime, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RemainingTime));
         break;

      case ITEM_TEMPERATURE_RUNNING_TIME:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RunningTime)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RunningTime, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.RunningTime));
         break;

      case ITEM_TEMPERATURE_DATETIME:
         assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.DateTime)== DataLen);
         memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.DateTime, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.DateTime));
         break;

      default:
         DEBUG("CForwarderMonitor::UpdateItem()----No this Item type\n");
         break;
   }
}

void CForwarderMonitor::SendForwarderData()
{
   DEBUG("CForwarderMonitor::SendForwarderData()\n");
   uint8 ForwarderPacketLen = 0;
   if(FORWARDER_TYPE_TEMPERATURE == m_ForwarderType)
   {
      DEBUG("CForwarderMonitor::SendForwarderData()---1\n");
      ForwarderPacketLen = FORWARDER_TYPE_TEMPERATURE_DATA_LEN - 2;
      uint8 ForwarderData[FORWARDER_TYPE_TEMPERATURE_DATA_LEN] = {0};
      ForwarderData[0] = VALVE_PACKET_FLAG;
      ForwarderData[1] = ForwarderPacketLen;
      m_ForwarderLock.Lock();
      for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++)
      {
         DEBUG("CForwarderMonitor::SendForwarderData()---2\n");
         if( ForwarderIter->second.IsOffline )
         {
            continue;
         }
         for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
         {
            if( (false == ValveIter->second.IsActive) || ValveIter->second.IsDataMissing )
            {
               DEBUG("CForwarderMonitor::SendForwarderData()----ForwarderID=0x%08x, ValveID=0x%04x NOTActive or DataMissing\n", ForwarderIter->first, ValveIter->first);
               continue;
            }
            memcpy(ForwarderData+2, ValveIter->second.ValveData.ValveTemperature.MacAddress, ForwarderPacketLen);
            DEBUG("CForwarderMonitor::SendForwarderData()----ForwarderID=0x%08x, ValveID=0x%04x\n", ForwarderIter->first, ValveIter->first);
            PrintData(ForwarderData, sizeof(ForwarderData));
            CPortal::GetInstance()->InsertForwarderData(ForwarderData, sizeof(ForwarderData));
         }
      }
      m_ForwarderLock.UnLock();
   }else
   {
      DEBUG("CForwarderMonitor::SendForwarderData()---3\n");
      ForwarderPacketLen = FORWARDER_TYPE_HEAT_DATA_LEN - 2;
      uint8 ForwarderData[FORWARDER_TYPE_HEAT_DATA_LEN] = {0};
      ForwarderData[0] = VALVE_PACKET_FLAG;
      ForwarderData[1] = ForwarderPacketLen;
      m_ForwarderLock.Lock();
      for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++)
      {
         DEBUG("CForwarderMonitor::SendForwarderData()---4\n");
         if( ForwarderIter->second.IsOffline )
         {
            continue;
         }
         for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
         {
            if( (false == ValveIter->second.IsActive) || ValveIter->second.IsDataMissing )
            {
               DEBUG("CForwarderMonitor::SendForwarderData()----ForwarderID=0x%08x, ValveID=0x%04x NOTActive or DataMissing\n", ForwarderIter->first, ValveIter->first);
               continue;
            }
            DEBUG("CForwarderMonitor::SendForwarderData()---5\n");
            memcpy(ForwarderData+2, ValveIter->second.ValveData.ValveHeat.MacAddress, ForwarderPacketLen);
            CPortal::GetInstance()->InsertForwarderData(ForwarderData, sizeof(ForwarderData));
         }
      }
      m_ForwarderLock.UnLock();
   }
}

bool CForwarderMonitor::GetStatus(StatusE& Status)
{
   DEBUG("CForwarderMonitor::GetStatus()\n");
   if( false == IsStarted() )
   {
      Status = STATUS_ERROR;
      DEBUG("CForwarderMonitor::GetStatus()----STATUS_ERROR\n");
   }else
   {
      m_ForwarderInfoListLock.Lock();
      bool IsOffline = true;
      for(ForwarderInfoListT::iterator ForwarderIter = m_ForwarderInfoList.begin(); ForwarderIter != m_ForwarderInfoList.end(); ForwarderIter++)
      {
         if( false == ForwarderIter->IsOffline )
         {
            IsOffline = false;
            break;
         }
      }
      m_ForwarderInfoListLock.UnLock();

      if( IsOffline )
      {
         DEBUG("CForwarderMonitor::GetStatus()----STATUS_OFFLINE\n");
         Status = STATUS_OFFLINE;
      }else
      {
         DEBUG("CForwarderMonitor::GetStatus()----STATUS_OK\n");
         Status = STATUS_OK;
      }
   }

   return true;
}

bool CForwarderMonitor::GetUserList(vector<user_t>& users) {
    if (users_lock.TryLock()) {
        users = this->users;
        users_lock.UnLock();
        return true;
    }
    return false;
}

void CForwarderMonitor::SetForwarderInfo()
{
   m_ForwarderInfoListLock.Lock();
   m_ForwarderInfoList.clear();
   uint32 i = 0;
   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++, i++)
   {
      ForwarderInfoT Temp;
      Temp.IsOffline = ForwarderIter->second.IsOffline;
      Temp.ForwarderID = ForwarderIter->first;
      Temp.A1ResponseStrLen = ForwarderIter->second.A1ResponseStrLen;
      memcpy(Temp.A1ResponseStr, ForwarderIter->second.A1ResponseStr, ForwarderIter->second.A1ResponseStrLen);
      Temp.ValveCount = ForwarderIter->second.ValveList.size();
      m_ForwarderInfoList.push_back(Temp);
   }
   m_ForwarderInfoListLock.UnLock();
}

bool CForwarderMonitor::GetForwarderInfo(ForwarderInfoListT& ForwarderInfoList)
{
   bool Ret = false;
   if(m_ForwarderInfoListLock.TryLock())
   {
      ForwarderInfoList = m_ForwarderInfoList;
      m_ForwarderInfoListLock.UnLock();
      Ret = true;
   }
   return Ret;
}

void CForwarderMonitor::GetForwarderInfoTask()
{
   DEBUG("CForwarderMonitor::GetForwarderInfoTask()-----m_ForwardInfoDataReady=%d\n", m_ForwardInfoDataReady);
   m_ForwarderLock.Lock();
   if( false == m_ForwardInfoDataReady )
   {
      SendA1();
   }

   if(m_ForwardInfoTimeOut.Done())
   {
      DEBUG("CForwarderMonitor::GetForwarderInfoTask()-----ForwardInfoData invalid\n");
      m_ForwardInfoDataReady = false;
   }

   m_ForwarderLock.UnLock();
}

void CForwarderMonitor::SaveForwarderMacUserIDTask() {
    DEBUG("CForwarderMonitor::SaveForwarderMacUserIDTask()----m_IsNewUserIDFound=%d\n", m_IsNewUserIDFound);
    LOGDB * db = dbopen("users", DB_NEW, sizeof(user_t));
    if (db == NULL) return;
    users_lock.Lock();
    users.clear();
    for (ForwarderMapT::iterator forwarderIter = m_DraftForwarderMap.begin(); forwarderIter != m_DraftForwarderMap.end(); forwarderIter++) {
        for (ValveListT::iterator valveIter = forwarderIter->second.ValveList.begin(); valveIter != forwarderIter->second.ValveList.end(); valveIter++) {
            if (0 == memcmp(INVALID_USERID, valveIter->second.ValveData.ValveTemperature.UserID, sizeof(valveIter->second.ValveData.ValveTemperature.UserID))) {
                continue;
            }
            user_t user;
            memcpy(user.uid, valveIter->second.ValveData.ValveTemperature.UserID, USERID_LEN);
            user.fid = forwarderIter->first;
            user.vid = valveIter->first;
            dbput(db, &user, sizeof(user_t));
            users.push_back(user);
        }
    }
    users_lock.UnLock();
    dbclose(db);
}

/*
void CForwarderMonitor::SaveForwarderMacUserIDTask()
{
   DEBUG("CForwarderMonitor::SaveForwarderMacUserIDTask()----m_IsNewUserIDFound=%d\n", m_IsNewUserIDFound);
   if(false == m_IsNewUserIDFound)
   {
      return;
   }

   const char* MAC_USERID_FILE_NAME = "Mac_UserID.txt";
   FILE* pMacUserIDFile = fopen(MAC_USERID_FILE_NAME, "w+");
   if(NULL == pMacUserIDFile)
   {
      return;
   }
   for(ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++)
   {
      for(ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++)
      {
         if( 0 == memcmp(INVALID_USERID, ValveIter->second.ValveData.ValveTemperature.UserID, sizeof(ValveIter->second.ValveData.ValveTemperature.UserID) ) )
         {
            continue;
         }
         char Temp[256] = {0};
         strcpy(Temp, "0x");
         uint32 i = 0;
         for( ;i < MACADDRESS_LEN; i++)
         {
            sprintf(Temp+strlen(Temp), "%02X", ValveIter->second.ValveData.ValveTemperature.MacAddress[i]);
         }
         sprintf(Temp+strlen(Temp), ":0x");
         for( i=0 ;i < USERID_LEN; i++)
         {
            sprintf(Temp+strlen(Temp), "%02X", ValveIter->second.ValveData.ValveTemperature.UserID[i]);
         }
         Temp[strlen(Temp)] = '\r';
         Temp[strlen(Temp)] = '\n';
         fputs(Temp, pMacUserIDFile);
      }
   }
   fclose(pMacUserIDFile);
   m_IsNewUserIDFound = false;
}
*/

void CForwarderMonitor::ConfigValve(uint8* pConfigStr, uint32 ConfigStrLen, ValveCtrlTypeE ValveCtrl, uint8& ValveConfigOKCount)
{
   if( (NULL == pConfigStr) || (0 == ConfigStrLen) )
   {
      return;
   }

   DEBUG("CForwarderMonitor::ConfigValve()\n");
   m_ForwarderLock.Lock();
   if( false == m_ForwardInfoDataReady )
   {
      SendA1();
   }

   ValveConfigOKCount = 0;
   uint8 VavleCtrlData[MAX_PACKET_LEN] = {0};
   uint32 Pos = 0;
   VavleCtrlData[Pos] = VALVE_CTRL_FLAG;
   Pos++;
   VavleCtrlData[Pos] = VALVE_CTRL_RANDAM;
   Pos++;
   VavleCtrlData[Pos] = 1/*function code*/+ConfigStrLen;
   Pos++;
   VavleCtrlData[Pos] = ValveCtrl;
   Pos++;
   memcpy(VavleCtrlData+Pos, pConfigStr, ConfigStrLen);
   Pos += ConfigStrLen;
   PrintData(VavleCtrlData, Pos);
   ValveConfigOKCount = SendValveCtrlOneByOne(VavleCtrlData, Pos);
   m_ForwarderLock.UnLock();
}

void CForwarderMonitor::QueryUser(uint8 uid[USERID_LEN], uint8 *data, uint16 len) {
    uint8 * cmd = (uint8 *)cbuffer_write(m_CardCmdBuf);
    if (cmd == NULL) {
        DEBUG("No enough memory for quering user\n");
        return;
    }

    for (vector<user_t>::iterator userIter = users.begin(); userIter != users.end(); userIter ++) {
        if (memcmp(userIter->uid, uid, USERID_LEN) == 0) {
            bzero(cmd, 1024);
            *(uint32 *)cmd = userIter->fid; cmd += sizeof(uint32);
            *(uint16 *)cmd = userIter->vid; cmd += sizeof(uint16);
            uint16 * cmdlen = (uint16 *)cmd; cmd += sizeof(uint16);
            cmd[0] = VALVE_CTRL_FLAG;
            cmd[1] = VALVE_CTRL_RANDAM;
            if (len < 0xFF) {
                cmd[2] = (1 + len) & 0xFF; // command + data
                cmd[3] = VALVE_CTRL_QUERY_USER;
                memcpy(cmd + 4, data, len);
                * cmdlen = 4 + len;
            } else {
                cmd[2] = 0xFF;
                cmd[3] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                cmd[4] = (1 + len) & 0xFF; // low byte for 'command + data'
                cmd[5] = VALVE_CTRL_QUERY_USER;
                memcpy(cmd + 6, data, len);
                * cmdlen = 6 + len;
            }
            cbuffer_write_done(m_CardCmdBuf);
        }
    }
}

void CForwarderMonitor::Prepaid(uint8 uid[USERID_LEN], uint8 *data, uint16 len) {
    uint8 * cmd = (uint8 *)cbuffer_write(m_CardCmdBuf);
    if (cmd == NULL) {
        DEBUG("No enough memory for prepaid\n");
        return;
    }

    for (vector<user_t>::iterator userIter = users.begin(); userIter != users.end(); userIter ++) {
        if (memcmp(userIter->uid, uid, USERID_LEN) == 0) {
            bzero(cmd, 1024);
            *(uint32 *)cmd = userIter->fid; cmd += sizeof(uint32);
            *(uint16 *)cmd = userIter->vid; cmd += sizeof(uint16);
            uint16 * cmdlen = (uint16 *)cmd; cmd += sizeof(uint16);
            cmd[0] = VALVE_CTRL_FLAG;
            cmd[1] = VALVE_CTRL_RANDAM;
            DEBUG("Prepaid command, sent to valve: ");
            if (len < 0xFF) {
                cmd[2] = (1 + len) & 0xFF; // command + data
                cmd[3] = VALVE_CTRL_PREPAID;
                memcpy(cmd + 4, data, len);
                * cmdlen = 4 + len;
                hexdump(cmd, (4 + len));
            } else {
                cmd[2] = 0xFF;
                cmd[3] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                cmd[4] = (1 + len) & 0xFF; // low byte for 'command + data'
                cmd[5] = VALVE_CTRL_PREPAID;
                memcpy(cmd + 6, data, len);
                * cmdlen = 6 + len;
                hexdump(cmd, (6 + len));
            }
            cbuffer_write_done(m_CardCmdBuf);
        }
    }
}

void CForwarderMonitor::SendCardHostCommand() {
    uint8 * data = (uint8 *) cbuffer_read(m_CardCmdBuf);
    if (data == NULL) return;
    uint32 fid = *(uint32 *)data; data += sizeof(uint32);
    uint16 vid = *(uint16 *)data; data += sizeof(uint16);
    uint16 cmdlen = *(uint16 *)data; data += sizeof(uint16);
    uint8 code = 0;

    if (data[2] == 0xFF) {
        code = data[5];
    } else {
        code = data[3];
    }

    if (!SendA2A3(data, cmdlen, fid, vid)) {
        cbuffer_read_done(m_CardCmdBuf);
        // SendA2A3 failed!
        switch (code) {
        case VALVE_CTRL_QUERY_USER:
            CCardHost::GetInstance()->AckQueryUser(NULL, 0);
            break;
        case VALVE_CTRL_PREPAID:
            CCardHost::GetInstance()->AckPrepaid(NULL, 0);
            break;
        default:
            break;
        }
        return;
    }
    cbuffer_read_done(m_CardCmdBuf);
}
