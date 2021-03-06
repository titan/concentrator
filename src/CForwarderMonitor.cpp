#include<assert.h>
#include"CForwarderMonitor.h"
#include"CPortal.h"
#include"Utils.h"
#include "CCardHost.h"
#include "logdb.h"

#ifdef DEBUG_FORWARDER
#include <time.h>
#define DEBUG(...) do {printf("%ld %s::%s %d ----", time(NULL), __FILE__, __func__, __LINE__);printf(__VA_ARGS__);} while(false)
#include "hexdump.h"
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
const uint8 FORWARDER_INTERVAL = 10;//3 seconds
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

void fuserseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen)
{
    vector<user_t> * users = (vector<user_t> *) params;
    user_t * user = (user_t *) data;
    (* users).push_back(* user);
}

void frecordseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen)
{
    map<uint32, record_t> * records = (map<uint32, record_t> *) params;
    uint32 * vmac = (uint32 *) key;
    record_t * record = (record_t *) data;
    (* records)[* vmac] = * record;
}

static bool IsFixTimeCommand(const uint8* pValveCtrl, const uint32 ValveCtrlLen)
{
    if ( (NULL == pValveCtrl) || (0 == ValveCtrlLen) ) {
        return false;
    }

    const uint8 FIX_TIME_DATA_COMMAND[] = {VALVE_GET_TEMPERATURE, VALVE_GET_TIME, VALVE_GET_TIME_DATA};
    const uint8 VALVE_CTRL_POS = 3;
    for (uint32 i = 0; i < sizeof(FIX_TIME_DATA_COMMAND)/sizeof(FIX_TIME_DATA_COMMAND[0]); i++) {
        if ( FIX_TIME_DATA_COMMAND[i] == pValveCtrl[VALVE_CTRL_POS] ) {
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
    if ( NULL == m_Instance ) {
        m_ForwarderLock.Lock();
        if ( NULL == m_Instance ) {
            m_Instance = new CForwarderMonitor();
        }
        m_ForwarderLock.UnLock();
    }
    return m_Instance;
}

CForwarderMonitor::CForwarderMonitor():
    valveDataType(VALVE_DATA_TYPE_TEMPERATURE)
    , m_DayNoonTimer(DAY_TYPE)
    // , m_DayNoonTimer(HOUR_TYPE)//just for test
    //, m_DayNoonTimer(MINUTE_TYPE)//just for test
    , m_ForwardInfoDataReady(false)
    , m_IsNewUserIDFound(false)
    , m_CardCmdBuf(cbuffer_create(10, 1024))
{

    if (access("data/", F_OK) == -1) {
        mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
}

bool CForwarderMonitor::Init(uint32 nStartTime, uint32 nInterval)
{
    return (m_RepeatTimer.Start(nInterval*60)) && (m_DayNoonTimer.Start(12/*noon*/));
}

void CForwarderMonitor::SetCom(CSerialComm* pCom)
{
    if ( (NULL == pCom) || (m_pCommController == pCom) ) {
        return;
    }

    m_pCommController = pCom;
}

uint32 CForwarderMonitor::Run()
{
    time_t last = 0;
    LoadUsers();
    LoadRecords();
    while (1) {
        ResetForwarderData();

        if (time(NULL) - last > 300 || (time(NULL) - last > 10 && users.size() == 0)) {
            SendA1();
            GetValveUserID();
            last = time(NULL);
        }
        if ( m_RepeatTimer.Done() ) {
            if (m_IsNewUserIDFound)
                SaveUsers();
            m_ForwarderLock.Lock();

            if (valveDataType == VALVE_DATA_TYPE_HEAT) {
                GetHeatData();
                if (GetValveRecord() > 0) {
                    //GetValveRunningTime();
                    //GetValveTemperature();
                    if (BatchGetRecordsData() > 0 && syncRecords) {
                        SaveRecords();
                    }
                }
            } else {
                GetPunctualData();
            }
            GetValveTime();

            m_ForwarderLock.UnLock();

            SendForwarderData();
        }

        SendCardHostCommand();

        GetForwarderInfoTask();

        DayNoonTimerTask();

        sleep(1);
    }
}

void CForwarderMonitor::SendA1()
{
    DEBUG("\n");
    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ ) {
        uint8 QueryCommand[MAX_PACKET_LEN] = {0};//query
        uint32 Pos = 0;
        DEBUG("Query forwarder\n");
        QueryCommand[FORWARDER_COMMAND_ID_POS] = COMMAND_A1;
        Pos = FORWARDER_ID_POS;
        memcpy(QueryCommand+Pos, &(ForwarderIter->first), FORWARDER_ID_LEN);
        Pos += FORWARDER_ID_LEN;
        if ( SendCommand(QueryCommand, Pos) ) {
            ForwarderIter->second.IsOffline = false;
        } else {
            ForwarderIter->second.IsOffline = true;
            DEBUG("Can't get valve address\n");
        }
    }

    //Forwarder Valve data is ready
    m_ForwardInfoDataReady = true;
    m_ForwardInfoTimeOut.StartTimer(FORWARDERINFO_TIMEOUT);
    SetForwarderInfo();
}

bool CForwarderMonitor::SendA2(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
    DEBUG("\n");
    if (NULL == pValveCtrl || 0 == ValveCtrlLen) {
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
    if ( SendCommand(A2Command, Pos) ) {
        return true;
    } else {
        return false;
    }
}

bool CForwarderMonitor::SendA3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
    DEBUG("\n");
    if (NULL == pValveCtrl || 0 == ValveCtrlLen) {
        assert(0);
        return false;
    }

    uint8 A3Command[MAX_PACKET_LEN] = {0};//query
    A3Command[FORWARDER_COMMAND_ID_POS] = COMMAND_A3;
    uint32 Pos = FORWARDER_ID_POS;
    memcpy(A3Command+Pos, &ForwarderID, FORWARDER_ID_LEN);
    Pos += FORWARDER_ID_LEN;
    assert(Pos == A3_VALVE_ID_POS);
    memcpy(A3Command+Pos, &ValveID, VALVE_ID_LEN);
    Pos += VALVE_ID_LEN;
    if ( SendCommand(A3Command, Pos) ) {
        return true;
    } else {
        return false;
    }
}

bool CForwarderMonitor::SendA2A3(const uint8* pValveCtrl, const uint32 ValveCtrlLen, uint32 ForwarderID, uint16 ValveID)
{
    DEBUG("\n");
    for (uint32 i = 0; i < MAX_A2_A3_RETRY_COUNT; i++) {
        if ( SendA2(pValveCtrl, ValveCtrlLen, ForwarderID, ValveID) ) {
            sleep(3);
            if ( SendA3(pValveCtrl, ValveCtrlLen, ForwarderID, ValveID) ) {
                DEBUG("ForwarderID(0x%8X) ValveID(%04X) OK\n", ForwarderID, ValveID);
                return true;
            }
        }
    }
    DEBUG("ForwarderID(0x%8X) ValveID(%04X) Error\n", ForwarderID, ValveID);
    return false;
}

void CForwarderMonitor::ResetForwarderData()//must firstly be called in each loop
{
    //DEBUG("\n");
    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++) {
        for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
            ValveIter->second.IsActive = false;
            ValveIter->second.IsDataMissing = false;
        }
    }
}

void CForwarderMonitor::GetValveTime()
{
    DEBUG("\n");
    if (false == m_ForwardInfoDataReady) {
        //SendA1();
        return;
    }
    const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_GET_TIME};
    SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::SetValveTime(uint32 fid, uint16 vid, tm * t)
{
    DEBUG("\n");
    uint8 buf[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x09/*Len*/, VALVE_SET_TIME, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
    uint8 ptr = 4;
    uint16 century = ((t->tm_year + 1900) / 100);
    uint16 year = (t->tm_year + 1900) % 100;
    buf[ptr] = DEC2BCD(century);
    ptr ++;
    buf[ptr] = DEC2BCD(year);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_mon + 1);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_mday);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_hour);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_min);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_sec);
    ptr ++;
    buf[ptr] = DEC2BCD(t->tm_wday);
    ptr ++;

    if (false == SendA2A3(buf, sizeof(buf), fid, vid)) {
        DEBUG("Set valve time failed, Forwarder=0x%08X, Valve=0x%04X\n", fid, vid);
    }
}

uint8 CForwarderMonitor::GetValveRecord()
{
    DEBUG("\n");
    if (false == m_ForwardInfoDataReady) {
        //SendA1();
        return 0;
    }
    const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_GET_VALVE_RECORD};
    return SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::GetValveTemperature()
{
    DEBUG("\n");
    if (false == m_ForwardInfoDataReady ) {
        //SendA1();
        return;
    }

    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ ) {
        for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
            if (false == ValveIter->second.IsActive) {
                DEBUG("ForwarderID=0x%08X, ValveID=0x%04X NOT active\n", ForwarderIter->first, ValveIter->first);
                continue;
            }

            if (ValveIter->second.IsDataMissing) {
                DEBUG("ForwarderID=0x%08X, ValveID=0x%04X Data missing\n", ForwarderIter->first, ValveIter->first);
                continue;
            }

            /*
              if(-1 == (int32)ValveIter->second.ValveRecord.LastTemperatureIndex)
              {
              ValveIter->second.IsDataMissing = true;
              DEBUG("ForwarderID=0x%08X, ValveID=0x%04X LastTemperatureIndex=-1\n", ForwarderIter->first, ValveIter->first);
              continue;
              }
            */

            if (BatchGetTemperatureRecords(ForwarderIter->first, ValveIter->first) == 0) {
                ValveIter->second.IsDataMissing = true;
            }
        }
    }
}

/*
void CForwarderMonitor::GetValveRunningTime()
{
   DEBUG("CForwarderMonitor::GetValveRunningTime()\n");
   const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01//Len//, VALVE_GET_TIME_DATA};
   SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}
*/

void CForwarderMonitor::GetValveUserID()
{
    DEBUG("\n");
    const uint8 ValveCommand[] = {VALVE_CTRL_USERID_FLAG, VALVE_CTRL_USERID_FLAG, 0x01};
    SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
}

void CForwarderMonitor::DayNoonTimerTask()
{
    if ( m_DayNoonTimer.Done() ) {
        DEBUG("\n");
        m_ForwarderLock.Lock();

        //get Valve Record
        if ( false == m_ForwardInfoDataReady ) {
            //SendA1();
            m_ForwarderLock.UnLock();
            return;
        }

        /*
        ClearValveRecord();
        const uint8 ValveCommand[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01//Len//, VALVE_GET_VALVE_RECORD};
        SendValveCtrlOneByOne(ValveCommand, sizeof(ValveCommand));
        */

        for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ ) {
            if ( ForwarderIter->second.IsOffline ) {
                DEBUG("0x%8x offline\n", ForwarderIter->first);
                continue;
            }
            for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
                if (false == ValveIter->second.IsActive) {
                    DEBUG("ForwarderID(0x%8X) ValveID(%04X) offline\n", ForwarderIter->first, ValveIter->first);
                    continue;
                }

                uint32 vmac = ((ForwarderIter->first & 0xFFFF) << 16) | ValveIter->first;
                record_t rec = records[vmac];

                // Get consume data
                BatchGetConsumeRecords(ForwarderIter->first, ValveIter->first);

                // Get charge data
                BatchGetRechargeRecords(ForwarderIter->first, ValveIter->first);

                if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                    // Get temperature data
                    BatchGetTemperatureRecords(ForwarderIter->first, ValveIter->first);
                }
            }
        }

        m_ForwarderLock.UnLock();
    }
}

/*
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
*/

bool CForwarderMonitor::SendCommand(uint8* pCommand, uint32 CommandLen)
{
    if (NULL == m_pCommController) {
        DEBUG("COMM is NULL\n");
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

    for (uint8 i=0; i<1; i++) {
        DEBUG("Send Command ");
        hexdump(SendCommand, Pos);
        if ( COMM_OK != m_pCommController->WriteBuf(SendCommand, Pos, FORWARDER_TIMEOUT) ) {
            DEBUG("WriteError\n");
            continue;
        }
        FlashLight(LIGHT_FORWARD);
        //myusleep(50 * 1000);

        uint8 Buffer[MAX_BUFFER_LEN] = {0};
        uint32 BufferCount = sizeof(Buffer);
        if (COMM_OK == m_pCommController->ReadBuf(Buffer
                , BufferCount
                , FORWARDER_COMMAND_BEGIN_FLAG
                , sizeof(FORWARDER_COMMAND_BEGIN_FLAG)
                , FORWARDER_COMMAND_END_FLAG
                , sizeof(FORWARDER_COMMAND_END_FLAG)
                , FORWARDER_TIMEOUT) ) {
            DEBUG("Readbuf %d bytes ", BufferCount);
            hexdump(Buffer, BufferCount);
            FlashLight(LIGHT_FORWARD);
            if (ParseData(Buffer+sizeof(FORWARDER_COMMAND_BEGIN_FLAG)
                          , BufferCount-sizeof(FORWARDER_COMMAND_BEGIN_FLAG)-sizeof(FORWARDER_COMMAND_END_FLAG)
                          , pCommand
                          , CommandLen)) {
                return true;
            } else {
                sleep(1);//myusleep(200 * 1000);
            }
        } else {
            DEBUG("ReadBuf timeout\n");
        }
    }
    return false;
}

bool CForwarderMonitor::AddForwareder(uint32 ForwarderID)
{
    DEBUG("\n");
    if ( m_DraftForwarderMap.end() != m_DraftForwarderMap.find(ForwarderID) ) {
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
    DEBUG("\n");
    if ( NULL == pReceiveData || ReceiveDataLen <= 0 ) {
        return false;
    }

    uint32 ReceiveForwarderID = 0;
    memcpy(&ReceiveForwarderID, pReceiveData+FORWARDER_ID_POS, sizeof(ReceiveForwarderID));
    assert(ReceiveForwarderID == SendForwarderID);
    uint32 Pos = A1_ACK_COMPOSITE_VALVE_COUNT_POS;
    uint8 CompositeValveCount = pReceiveData[Pos];
    Pos += VALVE_COUNT_LEN;

    DEBUG("SendForwarderID=0x%08x, CompositeValveCount=0x%02x\n", SendForwarderID, CompositeValveCount);
    for (uint8 CompositeValveIndex = 0; (CompositeValveIndex<CompositeValveCount) && (Pos<ReceiveDataLen); CompositeValveIndex++) {
        uint16 ValveID = 0;
        memcpy(&(ValveID), pReceiveData+Pos, VALVE_ID_LEN);
        if ( m_DraftForwarderMap[SendForwarderID].ValveList.end() == m_DraftForwarderMap[SendForwarderID].ValveList.find(ValveID) ) {
            //it is a new valve
            DEBUG("New valve-1-ReceiveForwarderID=0x%08x, ValveID=0x%04x\n", ReceiveForwarderID, ValveID);
            memset( &(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]), 0, sizeof(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]) );
        }
        m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].IsActive = true;
        if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
            memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress, &ReceiveForwarderID, 2);
            memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress+2, &ValveID, VALVE_ID_LEN);
            DEBUG("1--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                  , ReceiveForwarderID
                  , ValveID
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
        } else {
            memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress, &ReceiveForwarderID, 2);
            memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress+2, &ValveID, VALVE_ID_LEN);
            DEBUG("1--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                  , ReceiveForwarderID
                  , ValveID
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[0]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[1]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[2]
                  , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[3]);
        }
        Pos += VALVE_ID_LEN;
        uint8 ChildValveCount = pReceiveData[Pos];
        DEBUG("ChildValveCount=0x%02x\n", ChildValveCount);
        Pos += VALVE_COUNT_LEN;
        for (uint8 ChildValveIndex = 0; ChildValveIndex < ChildValveCount; ChildValveIndex++) {
            memcpy(&(ValveID), pReceiveData+Pos, VALVE_ID_LEN);
            if ( m_DraftForwarderMap[SendForwarderID].ValveList.end() == m_DraftForwarderMap[SendForwarderID].ValveList.find(ValveID) ) {
                //it is a new valve
                DEBUG("New valve-2-ReceiveForwarderID=0x%08x, ValveID=0x%04x\n", ReceiveForwarderID, ValveID);
                memset( &(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]), 0, sizeof(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID]) );
            }
            m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].IsActive = true;
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress, &ReceiveForwarderID, 2);
                memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress+2, &ValveID, VALVE_ID_LEN);
                DEBUG("2--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                      , ReceiveForwarderID
                      , ValveID
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[0]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[1]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[2]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveTemperature.MacAddress[3]);
            } else {
                memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress, &ReceiveForwarderID, 2);
                memcpy(m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress+2, &ValveID, VALVE_ID_LEN);
                DEBUG("2--ReceiveForwarderID=0x%08x, ValveID=0x%04x, MacAddress=0x%02x%02x%02x%02x\n"
                      , ReceiveForwarderID
                      , ValveID
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[0]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[1]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[2]
                      , m_DraftForwarderMap[SendForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[3]);
            }
            Pos += VALVE_ID_LEN;
        }
    }
    if (Pos != ReceiveDataLen) {
        DEBUG("Pos=%d, ReceiveDataLen=%d\n", Pos, ReceiveDataLen);
        assert(0);
        return false;
    }

    if ( ReceiveDataLen < sizeof(m_DraftForwarderMap[SendForwarderID].A1ResponseStr) ) {
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
    DEBUG("\n");
    if (NULL == pReceiveData || ReceiveDataLen <= 0 || NULL == pSendData || SendDataLen <= 0) {
        return false;
    }

    DEBUG("SendData ");
    hexdump(pSendData, SendDataLen);
    DEBUG("ReceiveData ");
    hexdump(pReceiveData, ReceiveDataLen);
    if (pReceiveData[FORWARDER_COMMAND_ID_POS] != pSendData[FORWARDER_COMMAND_ID_POS]) {
        DEBUG("Command NOT match Send(0x%02x)--Receive(0x%02x)\n", pSendData[FORWARDER_COMMAND_ID_POS], pReceiveData[FORWARDER_COMMAND_ID_POS]);
        return false;
    }

    switch (pReceiveData[FORWARDER_COMMAND_ID_POS]) {
    case COMMAND_A1: {
        uint32 SendForwarderID = 0;
        memcpy(&SendForwarderID, pSendData+FORWARDER_ID_POS, FORWARDER_ID_LEN);
        return ParseA1Ack(pReceiveData , ReceiveDataLen , SendForwarderID);
    }
    break;

    case COMMAND_A2: {
        assert(FORWARDER_COMMAND_ID_LEN+FORWARDER_ID_LEN  == ReceiveDataLen);
        if (FORWARDER_COMMAND_ID_LEN+FORWARDER_ID_LEN == ReceiveDataLen) {
            return true;
        } else {
            DEBUG("AckLen NOT correct Needed(%d)==ActualLen(%d)\n", FORWARDER_ID_LEN, ReceiveDataLen);
            hexdump(pReceiveData, ReceiveDataLen);
            return false;
        }
    }

    case COMMAND_A3: {
        if (VALVE_NULL == pReceiveData[A3_VALVE_CTRL_POS]) {
            DEBUG("No Valve data\n");
            return false;
        }

        uint32 ForwarderID = 0;
        memcpy(&ForwarderID, pReceiveData+FORWARDER_ID_POS, FORWARDER_ID_LEN);
        uint16 ValveID = 0;
        memcpy(&ValveID, pReceiveData+A3_VALVE_ID_POS, VALVE_ID_LEN);
        uint32 CalcLen = A3_VALVE_CTRL_POS+pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]+VALVE_CTRL_COMMAND_OFFSET;
        if (VALVE_CTRL_USERID_FLAG == pReceiveData[A3_VALVE_CTRL_POS]) {
            //get userID valve command
            CalcLen = A3_VALVE_CTRL_POS+3+pReceiveData[A3_VALVE_CTRL_POS+2];
            assert(pReceiveData[A3_VALVE_CTRL_POS+2] == USERID_LEN);
            if (ReceiveDataLen != CalcLen) {
                DEBUG("get UserID--ForwarderID=0x%08X, ValveID=0x%04X, Len NOT match\n", ForwarderID, ValveID);
                return false;
            }
            DEBUG("Got UserID: ");
            hexdump(pReceiveData+A3_VALVE_CTRL_POS + 3, USERID_LEN);
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_CTRL_POS+3, USERID_LEN, ITEM_TEMPERATURE_USER_ID);
            } else {
                UpdateHeatItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_CTRL_POS+3, USERID_LEN, ITEM_HEAT_USER_ID);
            }
            m_IsNewUserIDFound = true;
            return true;
        }

        uint8 A3CtrlRet = pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET];
        DEBUG("A3RetPos=%u, A3CtrlRet=0x%02X\n", A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET, A3CtrlRet);
        if (A3CtrlRet&0xE0) {//the high 3 bits = 0 when A2 command is exec successfully
            DEBUG("A3CtrlRet=0x%02X\n A2 command fails\n", A3CtrlRet);
            return false;
        }

        DEBUG("A3_VALVE_CTRL_POS=%u, Flag=0x%02X\n", A3_VALVE_CTRL_POS, pReceiveData[A3_VALVE_CTRL_POS]);
        switch (pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_COMMAND_OFFSET]) {
        case VALVE_GET_TIME: {
            hexdump( pReceiveData+A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS+VALVE_CTRL_LEN_OFFSET]);

            uint16 Year = ((pReceiveData[A3_VALVE_VALUE_POS]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS]&0x0F);
            Year = Year*100 + ((pReceiveData[A3_VALVE_VALUE_POS+1]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+1]&0x0F);
            uint8 Month = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)]&0x0F);
            uint8 Day = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)]&0x0F);
            uint8 Hour = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)]&0x0F);
            uint8 Minute = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)]&0x0F);
            uint8 Second = ((pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)+sizeof(Minute)]&0xF0)>>4)*10 + (pReceiveData[A3_VALVE_VALUE_POS+sizeof(Year)+sizeof(Month)+sizeof(Day)+sizeof(Hour)+sizeof(Minute)]&0x0F);

            uint32 utc = DateTime2TimeStamp(Year, Month, Day, Hour, Minute, Second);
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE)
                UpdateTemperatureItem(ForwarderID, ValveID, (uint8*)&utc, sizeof(utc), ITEM_TEMPERATURE_DATETIME);
            uint32 now = 0;
            if (GetLocalTimeStamp(now)) {
                now += 8*60*60; // convert to be Beijing Time
                if (CPortal::GetInstance()->timeReady && (now - utc > 60 || utc - now > 60)) {
                    tm t;
                    if (GetLocalTime(t, now))
                        SetValveTime(ForwarderID, ValveID, &t);
                }
            }

            return true;
        }
        case VALVE_GET_TEMPERATURE: {
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                uint32 vmac = ((ForwarderID & 0xFFFF) << 16) | ValveID;
                records[vmac].sentTemperature ++;
                records[vmac].sentTemperature %= 6000;
                syncRecords = true;

                const uint32 INDOOR_TEMPERATURE_OFFSET = 5;
                UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_INDOOR_TEMPERATURE);
                UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS+FORWARDER_TEMPERATURE_LEN, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_SET_TEMPERATURE);
                UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+INDOOR_TEMPERATURE_OFFSET+A3_VALVE_VALUE_POS+FORWARDER_TEMPERATURE_LEN+FORWARDER_TEMPERATURE_LEN, 1, ITEM_TEMPERATURE_VALVE_RUNNING_TIME);
            }
            return true;
        }
        /*
        case VALVE_GET_TIME_DATA:
        if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
            UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_TOTAL_TIME);
            UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS+FORWARDER_TIME_LEN, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_REMAINING_TIME);
            UpdateTemperatureItem(ForwarderID, ValveID, pReceiveData+A3_VALVE_VALUE_POS+FORWARDER_TIME_LEN+FORWARDER_TIME_LEN, FORWARDER_TIME_LEN, ITEM_TEMPERATURE_RUNNING_TIME);
        }
        return true;
        */
        case VALVE_SWITCH_VALVE:
            DEBUG("OK--VALVE_CTRL_SWITCH_VALVE\n");
            return true;
        case VALVE_SET_HEAT_TIME:
            DEBUG("OK--VALVE_CTRL_SET_HEAT_TIME\n");
            return true;
        case VALVE_CONFIG:
            DEBUG("OK--VALVE_CTRL_CONFIG\n");
            return true;
        case VALVE_GET_VALVE_RECORD: {
            DEBUG("OK--VALVE_CTRL_GET_VALVE_RECORD\n");
            uint32 vmac = ((ForwarderID & 0xFFFF) << 16) | ValveID;

            /*
            const uint8 CHARGE_RECORD_INDEX_POS = 0;
            const uint8 CONSUME_RECORD_INDEX_POS = 1;
            const uint8 TEMPERATURE_RECORD_INDEX_POS = 3;
            const uint8 STATUS_RECORD_INDEX_POS = 5;
            */

            const uint8 * data = pReceiveData + A3_VALVE_VALUE_POS;
            uint8 ptr = 0;

            if (records.count(vmac) == 0) {
                record_t rec;
                rec.recharge = data[ptr] - 1;
                ptr ++;
                rec.sentRecharge = 0;
                rec.consume = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                rec.sentConsume = 0;
                rec.temperature = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                rec.sentTemperature = 0;
                rec.status = data[ptr] - 1;
                ptr ++;
                rec.sentStatus = 0;
                records[vmac] = rec;
                syncRecords = true;
                DEBUG("VMAC new 0x%08X recharge: %d, sentRecharge: %d\n", vmac, rec.recharge, rec.sentRecharge);
                DEBUG("VMAC new 0x%08X consume: %d, sentConsume: %d\n", vmac, rec.consume, rec.sentConsume);
            } else {
                if (records[vmac].recharge != data[ptr] - 1) {
                    syncRecords = true;
                }
                records[vmac].recharge = data[ptr] - 1;
                ptr ++;
                if (records[vmac].consume != data[ptr] * 256 + data[ptr + 1] - 1) {
                    syncRecords = true;
                }
                records[vmac].consume = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                records[vmac].temperature = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                records[vmac].status = data[ptr] - 1;
                ptr ++;
                DEBUG("VMAC 0x%08X recharge: %d, sentRecharge: %d\n", vmac, records[vmac].recharge, records[vmac].sentRecharge);
                DEBUG("VMAC 0x%08X consume: %d, sentConsume: %d\n", vmac, records[vmac].consume, records[vmac].sentConsume);
            }
            return true;
        }
        case VALVE_GET_RECHARGE_DATA: {
            DEBUG("OK--VALVE_CTRL_GET_CHARGE_DATA\n");
            uint8 ChargePacketDataLen = FORWARDER_CHARGE_PACKET_LEN-2;
            uint8 ChargePacketData[FORWARDER_CHARGE_PACKET_LEN] = {0};
            uint32 Pos = 0;
            ChargePacketData[0] = VALVE_PACKET_FLAG;
            Pos++;
            ChargePacketData[1] = ChargePacketDataLen;
            Pos++;
            ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.find(ForwarderID);
            if ((ForwarderIter != m_DraftForwarderMap.end()) && (ForwarderIter->second.ValveList.end() != ForwarderIter->second.ValveList.find(ValveID))) {
                if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                    if (0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                        DEBUG("VALVE_CTRL_GET_CHARGE_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
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
                } else {
                    if (0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID, sizeof(INVALID_USERID))) {
                        DEBUG("VALVE_CTRL_GET_CHARGE_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[0]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[1]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[2]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[3]);
                        return false;
                    }
                    memcpy( ChargePacketData+Pos
                            , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID
                            , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID) );
                    Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID);

                    memcpy( ChargePacketData+Pos
                            , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress
                            , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress) );
                    Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress);
                }
                memcpy(ChargePacketData+Pos, pReceiveData+A3_VALVE_VALUE_POS, FORWARDER_CHARGE_PACKET_LEN-Pos);
                DEBUG("Insert recharge to CPortal ");
                hexdump(ChargePacketData, sizeof(ChargePacketData));
                CPortal::GetInstance()->InsertChargeData(ChargePacketData, sizeof(ChargePacketData));
                uint32 vmac = ((ForwarderID & 0xFFFF) << 16) | ValveID;
                records[vmac].sentRecharge ++;
                records[vmac].sentRecharge %= 20;
                syncRecords = true;
                DEBUG("VMAC 0x%08X New sentRecharge is %d\n", vmac, records[vmac].sentRecharge);
            }
            return true;
        }
        case VALVE_GET_CONSUME_DATA: {
            DEBUG("OK--VALVE_CTRL_GET_CONSUME_DATA\n");
            uint8 ConsumePacketDataLen = FORWARDER_CONSUME_PACKET_LEN-2;
            uint8 ConsumePacketData[FORWARDER_CONSUME_PACKET_LEN] = {0};
            uint32 Pos = 0;
            ConsumePacketData[0] = VALVE_PACKET_FLAG;
            Pos++;
            ConsumePacketData[1] = ConsumePacketDataLen;
            Pos++;
            ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.find(ForwarderID);
            if ((ForwarderIter != m_DraftForwarderMap.end()) && (ForwarderIter->second.ValveList.end() != ForwarderIter->second.ValveList.find(ValveID))) {
                if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                    if (0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                        DEBUG("VALVE_CTRL_GET_CONSUME_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
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
                } else {
                    if (0 == memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID, sizeof(INVALID_USERID))) {
                        DEBUG("VALVE_CTRL_GET_CONSUME_DATA--MacAddress=%02X%02X%02X%02X No userID\n"
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[0]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[1]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[2]
                              , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress[3]);
                        return false;
                    }

                    memcpy( ConsumePacketData+Pos
                            , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress
                            , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress) );
                    Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.MacAddress);

                    memcpy( ConsumePacketData+Pos
                            , m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID
                            , sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID) );
                    Pos += sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID);
                    DEBUG("userid ");
                    hexdump(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveHeat.UserID));
                }

                const uint8 CONSUME_DATA_OFFSET = 4;
                const uint8 CONSUME_DATA_LENGTH = 4;
                memcpy(ConsumePacketData+Pos, pReceiveData+A3_VALVE_VALUE_POS+CONSUME_DATA_OFFSET, CONSUME_DATA_LENGTH);
                const uint8 PACKET_CONSUME_DATA_LENGTH = 24;
                Pos += PACKET_CONSUME_DATA_LENGTH;
                DEBUG("consume data ");
                hexdump(pReceiveData+A3_VALVE_VALUE_POS+CONSUME_DATA_OFFSET, CONSUME_DATA_LENGTH);

                const uint8 CONSUME_DATA_DATETIME_OFFSET = 0;
                //const uint8 CONSUME_DATA_DATETIME_LENGTH = 4;
                uint8 DateTimeStr[7] = {0};
                DateTimeStr[6] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET];//yy
                DateTimeStr[5] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+1];//yy
                DateTimeStr[4] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+2];//mm
                DateTimeStr[3] = pReceiveData[A3_VALVE_VALUE_POS+CONSUME_DATA_DATETIME_OFFSET+3];//dd
                uint32 DateTime = DateTime2TimeStamp(DateTimeStr, sizeof(DateTimeStr));
                memcpy(ConsumePacketData+Pos, &DateTime, sizeof(DateTime));
                DEBUG("consume date ");
                hexdump(&DateTime, sizeof(DateTime));
                DEBUG("Insert consume to CPortal ");
                hexdump(ConsumePacketData, sizeof(ConsumePacketData));
                CPortal::GetInstance()->InsertConsumeData(ConsumePacketData, sizeof(ConsumePacketData));

                uint32 vmac = ((ForwarderID & 0xFFFF) << 16) | ValveID;
                records[vmac].sentConsume ++;
                records[vmac].sentConsume %= 2400;
                syncRecords = true;
                DEBUG("VMAC 0x%08X New sentConsume is %d\n", vmac, records[vmac].sentConsume);
            }
            return true;
        }
        case VALVE_GET_HEAT_DATA: {
            const uint8 * data = pReceiveData + A3_VALVE_VALUE_POS;
            const uint8 len = pReceiveData[A3_VALVE_CTRL_POS + VALVE_CTRL_LEN_OFFSET];
            uint8 ptr = 0;

            while (ptr < len && data[ptr] != 0x68) ptr ++; // skip preface
            if (ptr == len) {
                DEBUG("Invalid response from heat device\n");
                return false;
            }

            ptr ++; // skip frame start flag

            uint8 type = data[ptr];
            ptr ++;
            uint8 addr[7];
            memcpy(addr, data + ptr, 7);
            ptr += 7;
            uint8 code = data[ptr];
            ptr ++;
            uint8 dlen = data[ptr];
            ptr ++;
            uint16 identifier;
            memcpy(&identifier, data + ptr, 2);
            ptr += 2;
            uint8 sn = data[ptr];
            ptr ++;
            uint8 dayHeat[5], heat[5], heatPower[5], flow[5], totalFlow[5];
            memcpy(dayHeat, data + ptr, 5);
            ptr += 5;
            memcpy(heat, data + ptr, 5);
            ptr += 5;
            memcpy(heatPower, data + ptr, 5);
            ptr += 5;
            memcpy(flow, data + ptr, 5);
            ptr += 5;
            memcpy(totalFlow, data + ptr, 5);
            ptr += 5;
            uint8 inTemp[3], outTemp[3], workTime[3];
            memcpy(inTemp, data + ptr, 3);
            ptr += 3;
            memcpy(outTemp, data + ptr, 3);
            ptr += 3;
            memcpy(workTime, data + ptr, 3);
            ptr += 3;

            // date
            uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint32 utc = DateTime2TimeStamp(year, month, day, hour, minute, second);

            // device status
            uint16 status;
            memcpy(&status, data + ptr, 2);
            ptr += 2;

            UpdateHeatItem(ForwarderID, ValveID, &type, sizeof(uint8), ITEM_HEAT_TYPE);
            UpdateHeatItem(ForwarderID, ValveID, addr, HEAT_ADDRESS_LEN, ITEM_HEAT_ADDRESS);
            UpdateHeatItem(ForwarderID, ValveID, inTemp, HEAT_TEMPERATURE_LEN, ITEM_HEAT_INPUT_WATER_TEMPERATURE);
            UpdateHeatItem(ForwarderID, ValveID, outTemp, HEAT_TEMPERATURE_LEN, ITEM_HEAT_OUTPUT_WATER_TEMPERATURE);
            UpdateHeatItem(ForwarderID, ValveID, totalFlow, HEAT_FLOW_LEN, ITEM_HEAT_TOTAL_FLOW);
            UpdateHeatItem(ForwarderID, ValveID, heat, HEAT_HEAT_LEN, ITEM_HEAT_TOTAL_HEAT);
            UpdateHeatItem(ForwarderID, ValveID, workTime, HEAT_RUNNING_TIME_LEN, ITEM_HEAT_TOTAL_RUNNING_TIME);
            UpdateHeatItem(ForwarderID, ValveID, (uint8 *)&utc, sizeof(uint32), ITEM_HEAT_DATETIME);

            return true;
        }
        case VALVE_GET_PUNCTUAL_DATA: {
            const uint8 * data = pReceiveData + A3_VALVE_VALUE_POS;
            //uint8 len = pReceiveData[A3_VALVE_CTRL_POS + VALVE_CTRL_LEN_OFFSET];
            uint8 ptr = 0;

            // transcation date
            uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint32 utc = DateTime2TimeStamp(year, month, day, hour, 0, 0);
            // UpdateTemperatureItem(ForwarderID, ValveID, (uint8*)&utc, sizeof(uint32), ITEM_TEMPERATURE_DATETIME);

            // temperatures
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE)
                UpdateTemperatureItem(ForwarderID, ValveID, data + ptr, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_INDOOR_TEMPERATURE);
            ptr += FORWARDER_TEMPERATURE_LEN;
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE)
                UpdateTemperatureItem(ForwarderID, ValveID, data + ptr, FORWARDER_TEMPERATURE_LEN, ITEM_TEMPERATURE_SET_TEMPERATURE);
            ptr += FORWARDER_TEMPERATURE_LEN;

            // device running time
            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE)
                UpdateTemperatureItem(ForwarderID, ValveID, data + ptr, 1, ITEM_TEMPERATURE_VALVE_RUNNING_TIME);
            ptr += FORWARDER_TIME_LEN;

            ptr += 2; // skip device status
            const uint8 * uid = data + ptr;
            ptr += USERID_LEN;
            for (vector<user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
                if (memcmp(uid, iter->uid.x, USERID_LEN) == 0) {
                    if (ValveID != (iter->vmac & 0xFFFF)) {
                        if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE)
                            UpdateTemperatureItem(ForwarderID, ValveID, uid, USERID_LEN, ITEM_TEMPERATURE_USER_ID);
                        m_IsNewUserIDFound = true;
                    }
                }
            }

            // device time
            year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;

            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                utc = DateTime2TimeStamp(year, month, day, hour, minute, second);
                UpdateTemperatureItem(ForwarderID, ValveID, (uint8*)&utc, sizeof(uint32), ITEM_TEMPERATURE_DATETIME);
            }

            uint32 vmac = ((ForwarderID & 0xFFFF) << 16) | ValveID;

            if (records.count(vmac) == 0) {
                record_t rec;
                rec.recharge = data[ptr] - 1;
                ptr ++;
                rec.sentRecharge = 0;
                rec.consume = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                rec.sentConsume = 0;
                rec.temperature = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                rec.sentTemperature = 0;
                rec.status = data[ptr] - 1;
                ptr ++;
                rec.sentStatus = 0;
                records[vmac] = rec;
                syncRecords = true;
            } else {
                if (records[vmac].recharge != data[ptr] - 1) {
                    syncRecords = true;
                }
                records[vmac].recharge = data[ptr] - 1;
                ptr ++;
                if (records[vmac].consume != data[ptr] * 256 + data[ptr + 1] - 1) {
                    syncRecords = true;
                }
                records[vmac].consume = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                records[vmac].temperature = data[ptr] * 256 + data[ptr + 1] - 1;
                ptr += 2;
                records[vmac].status = data[ptr] - 1;
                ptr ++;
            }

            /*
            // charge data pointer
            m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastChargeIndex = data[ptr] - 1; ptr ++;
            // consume data pointer
            m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastConsumeIndex = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
            // temperature data pointer
            m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveRecord.LastTemperatureIndex = data[ptr] * 256 + pReceiveData[ptr + 1] - 1; ptr += 2;
            // missing device status pointer
            */

            return true;
        }
        case VALVE_QUERY_USER: {
            // it's query user command response
            ForwarderMapT::iterator iter = m_DraftForwarderMap.find(ForwarderID);
            if ((iter != m_DraftForwarderMap.end()) && (iter->second.ValveList.end() != iter->second.ValveList.find(ValveID))) {
                if (0 != memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                    userid_t uid;
                    memcpy(uid.x, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(userid_t));
                    CCardHost::GetInstance()->AckQueryUser(uid, (uint8 *)pReceiveData + A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS + VALVE_CTRL_LEN_OFFSET]);
                    return true;
                }
            }
            return false;
        }
        case VALVE_RECHARGE: {
            // it's recharge command response
            ForwarderMapT::iterator iter = m_DraftForwarderMap.find(ForwarderID);
            if ((iter != m_DraftForwarderMap.end()) && (iter->second.ValveList.end() != iter->second.ValveList.find(ValveID))) {
                if (0 != memcmp(INVALID_USERID, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                    userid_t uid;
                    memcpy(uid.x, m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(userid_t));
                    CCardHost::GetInstance()->AckRecharge(uid, (uint8 *)pReceiveData + A3_VALVE_VALUE_POS, pReceiveData[A3_VALVE_CTRL_POS + VALVE_CTRL_LEN_OFFSET]);
                    return true;
                }
            }
            return false;
        }
        }
        break;
    }
    }
    return false;
}

uint8 CForwarderMonitor::SendValveCtrlOneByOne(const uint8* pValveCtrl, const uint32 ValveCtrlLen)
{
    DEBUG("\n");
    if (NULL == pValveCtrl || ValveCtrlLen <= 0) {
        DEBUG("Parameter error\n");
        return 0;
    }
    uint8 ValveAckCount = 0;

    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ ) {
        if ( ForwarderIter->second.IsOffline ) {
            DEBUG("0x%08x offline\n", ForwarderIter->first);
            continue;
        }
        for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
            if (false == ValveIter->second.IsActive) {
                DEBUG("ForwarderID(0x%8X) ValveID(%04X) offline\n", ForwarderIter->first, ValveIter->first);
                continue;
            }

            if ( IsFixTimeCommand(pValveCtrl, ValveCtrlLen) ) {
                if (ValveIter->second.IsDataMissing) {
                    DEBUG("ForwarderID(0x%8X) ValveID(%04X) data missing\n", ForwarderIter->first, ValveIter->first);
                    continue;
                }
            }

            const uint8 GetUserIDCommand[] = {VALVE_CTRL_USERID_FLAG, VALVE_CTRL_USERID_FLAG, 0x01};
            if ( (ValveCtrlLen == sizeof(GetUserIDCommand)) && (0 == memcmp(GetUserIDCommand, pValveCtrl, sizeof(GetUserIDCommand))) ) {
                DEBUG("UserID\n");
                if ( memcmp(ValveIter->second.ValveData.ValveTemperature.UserID, INVALID_USERID, sizeof(INVALID_USERID)) ) {
                    DEBUG("ForwarderID(0x%08X) ValveID(0x%04X) UserID already exists, no need to reget\n", ForwarderIter->first, ValveIter->first);
                    continue;
                }
            }

            if ( SendA2A3(pValveCtrl, ValveCtrlLen, ForwarderIter->first, ValveIter->first) ) {
                DEBUG("ForwarderID(0x%08X) ValveID(0x%04X) OK\n", ForwarderIter->first, ValveIter->first);
                ValveAckCount++;
            } else {
                if ( IsFixTimeCommand(pValveCtrl, ValveCtrlLen) ) {
                    DEBUG("ForwarderID(0x%08X) ValveID(0x%04X) Data missing\n", ForwarderIter->first, ValveIter->first);
                    ValveIter->second.IsDataMissing = true;
                }
                if ( (ValveCtrlLen == sizeof(GetUserIDCommand)) && (0 == memcmp(GetUserIDCommand, pValveCtrl, sizeof(GetUserIDCommand))) ) {
                    DEBUG("ForwarderID(0x%8X) ValveID(%04X) UserID missing\n", ForwarderIter->first, ValveIter->first);
                    ValveIter->second.IsDataMissing = true;
                }
            }
        }
    }

    return ValveAckCount;
}

void CForwarderMonitor::UpdateTemperatureItem(uint32 ForwarderID, uint16 ValveID, const uint8* pData, uint32 DataLen, ValveTemperatureTypeE ValveTemperatureType)
{
    DEBUG("\n");
    if ( NULL == pData || 0 >= DataLen ) {
        return;
    }
    DEBUG("Temperature(%d)--ForwarderID=0x%08x, ValveID=0x%04x\n", ValveTemperatureType, ForwarderID, ValveID);
    hexdump(pData, DataLen);

    switch (ValveTemperatureType) {
    case ITEM_TEMPERATURE_USER_ID:
        assert(sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID)== DataLen);
        memcpy(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, pData, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID));
        DEBUG("UserID--ForwarderID=0x%08x, ValveID=0x%04x\n", ForwarderID, ValveID);
        hexdump(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID, sizeof(m_DraftForwarderMap[ForwarderID].ValveList[ValveID].ValveData.ValveTemperature.UserID));
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
        DEBUG("No this Item type\n");
        break;
    }
}

void CForwarderMonitor::UpdateHeatItem(uint32 fid, uint16 vid, const uint8 * data, uint32 len, ValveHeatTypeE type)
{
    DEBUG("\n");
    if ( NULL == data || 0 >= len ) {
        return;
    }

    switch (type) {
    case ITEM_HEAT_MAC_ADDRESS:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.MacAddress, data, len);
        DEBUG("HeatType(ITEM_HEAT_MAC_ADDRESS)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_USER_ID:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.UserID, data, len);
        DEBUG("HeatType(ITEM_HEAT_USER_ID)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_TYPE:
        memcpy(&m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.HeatType, data, len);
        DEBUG("HeatType(ITEM_HEAT_TYPE)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_INPUT_WATER_TEMPERATURE:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.InputWaterTemperature, data, len);
        DEBUG("HeatType(ITEM_HEAT_INPUT_WATER_TEMPERATURE)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_OUTPUT_WATER_TEMPERATURE:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.OutputWaterTemperature, data, len);
        DEBUG("HeatType(ITEM_HEAT_OUTPUT_WATER_TEMPERATURE)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_TOTAL_FLOW:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.TotalFlow, data, len);
        DEBUG("HeatType(ITEM_HEAT_TOTAL_FLOW)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_TOTAL_HEAT:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.TotalHeat, data, len);
        DEBUG("HeatType(ITEM_HEAT_TOTAL_HEAT)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_TOTAL_RUNNING_TIME:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.TotalRunningTime, data, len);
        DEBUG("HeatType(ITEM_HEAT_TOTAL_RUNNING_TIME)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    case ITEM_HEAT_DATETIME:
        memcpy(m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveHeat.DateTime, data, len);
        DEBUG("HeatType(ITEM_HEAT_DATETIME)--ForwarderID=0x%04x, ValveID=0x%02x\n", fid, vid);
        break;

    default:
        DEBUG("Unknown ValveHeatType(%d)\n", type);
        break;
    }
    hexdump(data, len);
}

void CForwarderMonitor::SendForwarderData()
{
    DEBUG("\n");
    uint8 ForwarderPacketLen = 0;
    if (VALVE_DATA_TYPE_TEMPERATURE == valveDataType) {
        ForwarderPacketLen = FORWARDER_TYPE_TEMPERATURE_DATA_LEN - 2;
        uint8 ForwarderData[FORWARDER_TYPE_TEMPERATURE_DATA_LEN] = {0};
        ForwarderData[0] = VALVE_PACKET_FLAG;
        ForwarderData[1] = ForwarderPacketLen;
        m_ForwarderLock.Lock();
        for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++) {
            if ( ForwarderIter->second.IsOffline ) {
                continue;
            }
            for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
                if ( (false == ValveIter->second.IsActive) || ValveIter->second.IsDataMissing ) {
                    DEBUG("ForwarderID=0x%08x, ValveID=0x%04x NOTActive or DataMissing\n", ForwarderIter->first, ValveIter->first);
                    continue;
                }
                memcpy(ForwarderData+2, ValveIter->second.ValveData.ValveTemperature.MacAddress, ForwarderPacketLen);
                DEBUG("ForwarderID=0x%08x, ValveID=0x%04x\n", ForwarderIter->first, ValveIter->first);
                hexdump(ForwarderData, sizeof(ForwarderData));
                CPortal::GetInstance()->InsertValveData(ForwarderData, sizeof(ForwarderData));
            }
        }
        m_ForwarderLock.UnLock();
    } else {
        ForwarderPacketLen = FORWARDER_TYPE_HEAT_DATA_LEN - 2;
        uint8 ForwarderData[FORWARDER_TYPE_HEAT_DATA_LEN] = {0};
        ForwarderData[0] = VALVE_PACKET_FLAG;
        ForwarderData[1] = ForwarderPacketLen;
        m_ForwarderLock.Lock();
        for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++) {
            if ( ForwarderIter->second.IsOffline ) {
                continue;
            }
            for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
                if ( (false == ValveIter->second.IsActive) || ValveIter->second.IsDataMissing ) {
                    DEBUG("ForwarderID=0x%08x, ValveID=0x%04x NOTActive or DataMissing\n", ForwarderIter->first, ValveIter->first);
                    continue;
                }
                memcpy(ForwarderData+2, ValveIter->second.ValveData.ValveHeat.MacAddress, ForwarderPacketLen);
                CPortal::GetInstance()->InsertValveData(ForwarderData, sizeof(ForwarderData));
            }
        }
        m_ForwarderLock.UnLock();
    }
}

Status CForwarderMonitor::GetStatus()
{
    DEBUG("\n");
    if (false == IsStarted()) {
        DEBUG("STATUS_ERROR\n");
        return STATUS_ERROR;
    } else {
        m_ForwarderInfoListLock.Lock();
        bool IsOffline = true;
        for (ForwarderInfoListT::iterator ForwarderIter = m_ForwarderInfoList.begin(); ForwarderIter != m_ForwarderInfoList.end(); ForwarderIter++) {
            if (false == ForwarderIter->IsOffline) {
                IsOffline = false;
                break;
            }
        }
        m_ForwarderInfoListLock.UnLock();

        if (IsOffline) {
            DEBUG("STATUS_OFFLINE\n");
            return STATUS_OFFLINE;
        } else {
            DEBUG("STATUS_OK\n");
            return STATUS_OK;
        }
    }
}

bool CForwarderMonitor::GetUserList(vector<user_t>& users)
{
    DEBUG("\n");
    if (users_lock.TryLock()) {
        users = this->users;
        users_lock.UnLock();
        return true;
    }
    return false;
}

void CForwarderMonitor::SetForwarderInfo()
{
    DEBUG("\n");
    m_ForwarderInfoListLock.Lock();
    m_ForwarderInfoList.clear();
    uint32 i = 0;
    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++, i++) {
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
    DEBUG("\n");
    bool Ret = false;
    if (m_ForwarderInfoListLock.TryLock()) {
        ForwarderInfoList = m_ForwarderInfoList;
        m_ForwarderInfoListLock.UnLock();
        Ret = true;
    }
    return Ret;
}

void CForwarderMonitor::GetForwarderInfoTask()
{
    //DEBUG("\n");
    //DEBUG("m_ForwardInfoDataReady=%d\n", m_ForwardInfoDataReady);
    m_ForwarderLock.Lock();
    if ( false == m_ForwardInfoDataReady ) {
        //SendA1();
        return;
    }

    if (m_ForwardInfoTimeOut.Done()) {
        DEBUG("ForwardInfoData invalid\n");
        m_ForwardInfoDataReady = false;
    }

    m_ForwarderLock.UnLock();
}

uint16 CForwarderMonitor::ConfigValve(ValveCtrlType ValveCtrl, uint8* pConfigStr, uint16 ConfigStrLen)
{
    DEBUG("\n");
    uint16 ValveConfigOKCount = 0;
    if ( (NULL == pConfigStr) || (0 == ConfigStrLen) ) {
        return ValveConfigOKCount;
    }
    m_ForwarderLock.Lock();
    if ( false == m_ForwardInfoDataReady ) {
        //SendA1();
        return 0;
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
    hexdump(VavleCtrlData, Pos);
    ValveConfigOKCount = SendValveCtrlOneByOne(VavleCtrlData, Pos);
    m_ForwarderLock.UnLock();
    return ValveConfigOKCount;
}

void CForwarderMonitor::QueryUser(userid_t uid, uint8 *data, uint16 len)
{
    DEBUG("\n");
    uint8 * cmd = (uint8 *)cbuffer_write(m_CardCmdBuf);
    if (cmd == NULL) {
        DEBUG("No enough memory for quering user\n");
        return;
    }

    for (vector<user_t>::iterator userIter = users.begin(); userIter != users.end(); userIter ++) {
        if (memcmp(userIter->uid.x, uid.x, USERID_LEN) == 0) {
            bzero(cmd, 1024);
            *(uint32 *)cmd = userIter->fid;
            cmd += sizeof(uint32);
            *(uint16 *)cmd = (uint16)(userIter->vmac & 0xFFFF);
            cmd += sizeof(uint16);
            uint16 * cmdlen = (uint16 *)cmd;
            cmd += sizeof(uint16);
            cmd[0] = VALVE_CTRL_FLAG;
            cmd[1] = VALVE_CTRL_RANDAM;
            if ((1 + len) < 0xFF) {
                cmd[2] = (1 + len) & 0xFF; // command + data
                cmd[3] = VALVE_QUERY_USER;
                memcpy(cmd + 4, data, len);
                * cmdlen = 4 + len;
            } else {
                cmd[2] = 0xFF;
                cmd[3] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                cmd[4] = (1 + len) & 0xFF; // low byte for 'command + data'
                cmd[5] = VALVE_QUERY_USER;
                memcpy(cmd + 6, data, len);
                * cmdlen = 6 + len;
            }
            cbuffer_write_done(m_CardCmdBuf);
            return;
        }
    }
}

void CForwarderMonitor::Recharge(userid_t uid, uint8 *data, uint16 len)
{
    DEBUG("\n");
    uint8 * cmd = (uint8 *)cbuffer_write(m_CardCmdBuf);
    if (cmd == NULL) {
        DEBUG("No enough memory for recharge\n");
        return;
    }

    for (vector<user_t>::iterator userIter = users.begin(); userIter != users.end(); userIter ++) {
        if (memcmp(userIter->uid.x, uid.x, USERID_LEN) == 0) {
            bzero(cmd, 1024);
            *(uint32 *)cmd = userIter->fid;
            cmd += sizeof(uint32);
            *(uint16 *)cmd = (uint16)(userIter->vmac & 0xFFFF);
            cmd += sizeof(uint16);
            uint16 * cmdlen = (uint16 *)cmd;
            cmd += sizeof(uint16);
            cmd[0] = VALVE_CTRL_FLAG;
            cmd[1] = VALVE_CTRL_RANDAM;
            DEBUG("Recharge command, sent to valve: ");
            if ((1 + len) < 0xFF) {
                cmd[2] = (1 + len) & 0xFF; // command + data
                cmd[3] = VALVE_RECHARGE;
                memcpy(cmd + 4, data, len);
                * cmdlen = 4 + len;
                hexdump(cmd, (4 + len));
            } else {
                cmd[2] = 0xFF;
                cmd[3] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                cmd[4] = (1 + len) & 0xFF; // low byte for 'command + data'
                cmd[5] = VALVE_RECHARGE;
                memcpy(cmd + 6, data, len);
                * cmdlen = 6 + len;
                hexdump(cmd, (6 + len));
            }
            cbuffer_write_done(m_CardCmdBuf);
            return;
        }
    }
}

void CForwarderMonitor::SendCardHostCommand()
{
    //DEBUG("\n");
    userid_t uid;
    uint8 * data = (uint8 *) cbuffer_read(m_CardCmdBuf);
    if (data == NULL) return;
    uint32 fid = *(uint32 *)data;
    data += sizeof(uint32);
    uint16 vid = *(uint16 *)data;
    data += sizeof(uint16);
    uint16 cmdlen = *(uint16 *)data;
    data += sizeof(uint16);
    uint8 code = 0;

    bzero(uid.x, sizeof(userid_t));

    if (data[2] == 0xFF) {
        code = data[5];
    } else {
        code = data[3];
    }

    if (!SendA2A3(data, cmdlen, fid, vid)) {
        cbuffer_read_done(m_CardCmdBuf);
        // SendA2A3 failed!
        switch (code) {
        case VALVE_QUERY_USER: {
            ForwarderMapT::iterator iter = m_DraftForwarderMap.find(fid);
            if ((iter != m_DraftForwarderMap.end()) && (iter->second.ValveList.end() != iter->second.ValveList.find(vid))) {
                if (0 != memcmp(INVALID_USERID, m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                    memcpy(uid.x, m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveTemperature.UserID, sizeof(userid_t));
                }
            }
            CCardHost::GetInstance()->AckQueryUser(uid, NULL, 0);
            break;
        }
        case VALVE_RECHARGE: {
            ForwarderMapT::iterator iter = m_DraftForwarderMap.find(fid);
            if ((iter != m_DraftForwarderMap.end()) && (iter->second.ValveList.end() != iter->second.ValveList.find(vid))) {
                if (0 != memcmp(INVALID_USERID, m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveTemperature.UserID, sizeof(INVALID_USERID))) {
                    memcpy(uid.x, m_DraftForwarderMap[fid].ValveList[vid].ValveData.ValveTemperature.UserID, sizeof(userid_t));
                }
            }
            CCardHost::GetInstance()->AckRecharge(uid, NULL, 0);
            break;
        }
        default:
            break;
        }
        return;
    }
    cbuffer_read_done(m_CardCmdBuf);
}

void CForwarderMonitor::GetPunctualData()
{
    DEBUG("\n");
    if ( false == m_ForwardInfoDataReady ) {
        //SendA1();
        return;
    }

    //ClearValveRecord();
    const uint8 cmd[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x01/*Len*/, VALVE_GET_PUNCTUAL_DATA};
    SendValveCtrlOneByOne(cmd, sizeof(cmd));
}

void CForwarderMonitor::GetHeatData()
{
    DEBUG("\n");
    if ( false == m_ForwardInfoDataReady ) {
        //SendA1();
        return;
    }
    const uint8 cmd[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x011/*Len*/, VALVE_GET_HEAT_DATA, 0x68, 0x20, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x01, 0x03, 0x90, 0x1F, 0x0B, 0xEC, 0x16};
    SendValveCtrlOneByOne(cmd, sizeof(cmd));
}

void CForwarderMonitor::LoadUsers()
{
    DEBUG("\n");
    LOGDB * db = dbopen((char *)"data/users", DB_RDONLY, genuserkey);
    if (db != NULL) {
        dbseq(db, fuserseq, &users, sizeof(users));
        dbclose(db);
    }
}

void CForwarderMonitor::SaveUsers()
{
    DEBUG("\n");
    LOGDB * db = dbopen((char *)"data/users", DB_NEW, genuserkey);
    if (db != NULL) {
        users_lock.Lock();
        users.clear();
        for (ForwarderMapT::iterator forwarderIter = m_DraftForwarderMap.begin(); forwarderIter != m_DraftForwarderMap.end(); forwarderIter++) {
            for (ValveListT::iterator valveIter = forwarderIter->second.ValveList.begin(); valveIter != forwarderIter->second.ValveList.end(); valveIter++) {
                if (0 == memcmp(INVALID_USERID, valveIter->second.ValveData.ValveTemperature.UserID, sizeof(valveIter->second.ValveData.ValveTemperature.UserID))) {
                    continue;
                }
                user_t user;
                memcpy(user.uid.x, valveIter->second.ValveData.ValveTemperature.UserID, USERID_LEN);
                user.fid = forwarderIter->first;
                user.vmac = ((user.fid & 0xFFFF) << 16) | valveIter->first;
                dbput(db, &user.vmac, sizeof(uint32), &user, sizeof(user_t));
                users.push_back(user);
            }
        }
        users_lock.UnLock();

        dbclose(db);
        m_IsNewUserIDFound = false;
    }
}

void CForwarderMonitor::LoadRecords()
{
    DEBUG("\n");
    LOGDB * db = dbopen((char *)"data/records", DB_RDONLY, genrecordkey);
    if (db != NULL) {
        dbseq(db, frecordseq, &records, sizeof(records));
        dbclose(db);
    }
}

void CForwarderMonitor::SaveRecords()
{
    DEBUG("\n");
    LOGDB * db = dbopen((char *)"data/records", DB_NEW, genrecordkey);
    if (db != NULL) {
        for (map<uint32, record_t>::iterator iter = records.begin(); iter != records.end(); iter ++) {
            dbput(db, (void *)&iter->first, sizeof(uint32), &iter->second, sizeof(record_t));
        }
        dbclose(db);
        syncRecords = false;
    }
}

uint16 CForwarderMonitor::BatchGetConsumeRecords(uint32 fid, uint16 vid)
{
    DEBUG("\n");
    uint32 vmac = ((fid & 0xFFFF) << 16) | vid;
    record_t * rec = &records[vmac];
    uint8 cmd[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x03, VALVE_GET_CONSUME_DATA, 0xFF, 0xFF/*a invalid index*/};
    uint16 counter = 0;

    //Get consume data
    while (rec->sentConsume != rec->consume & 0x7FFF) {
        cmd[4] = ((rec->sentConsume + 1) % 2400) /256;
        cmd[5] = ((rec->sentConsume + 1) % 2400) % 256;
        DEBUG("VMAC 0x%08X Read no.%d consume record until no.%d --ForwarderID=0x%08X, ValveID=0x%04X\n", vmac, (rec->sentConsume + 1) % 2400, rec->consume & 0x7FFF, fid, vid);
        if (false == SendA2A3(cmd, sizeof(cmd), fid, vid)) {
            DEBUG("No more consume record to read--ForwarderID=0x%08X, ValveID=0x%04X\n", fid, vid);
            return counter;
        }
        counter ++;
    }

    return counter;
}

uint16 CForwarderMonitor::BatchGetRechargeRecords(uint32 fid, uint16 vid)
{
    DEBUG("\n");
    uint32 vmac = ((fid & 0xFFFF) << 16) | vid;
    record_t * rec = &records[vmac];
    uint16 counter = 0;

    //Get recharge data
    uint8 cmd[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x02, VALVE_GET_RECHARGE_DATA, 0xFF/*a invalid index*/};
    while (rec->sentRecharge != rec->recharge & 0x7F) {
        cmd[4] = (rec->sentRecharge + 1) % 20;
        DEBUG("VMAC 0x%08X Read no.%d recharge record--ForwarderID=0x%08X, ValveID=0x%04X\n", vmac, (rec->sentRecharge + 1) % 20, fid, vid);
        if (false == SendA2A3(cmd, sizeof(cmd), fid, vid)) {
            DEBUG("No more recharge record to read--ForwarderID=0x%08X, ValveID=0x%04X\n", fid, vid);
            return counter;
        }
        counter ++;
    }

    return counter;
}

uint16 CForwarderMonitor::BatchGetTemperatureRecords(uint32 fid, uint16 vid)
{
    DEBUG("\n");
    uint32 vmac = ((fid & 0xFFFF) << 16) | vid;
    record_t * rec = &records[vmac];
    uint16 counter = 0;

    //Get temperature data
    uint8 cmd[] = {VALVE_CTRL_FLAG, VALVE_CTRL_RANDAM, 0x03, VALVE_GET_TEMPERATURE, 0xFF, 0xFF/*a invalid index*/};
    while (rec->sentTemperature != rec->temperature & 0x7FFF) {
        cmd[4] = (rec->sentTemperature + 1) % 6000;
        DEBUG("Read no.%d temperature record--ForwarderID=0x%08X, ValveID=0x%04X\n", (rec->sentTemperature + 1) % 6000, fid, vid);
        if (false == SendA2A3(cmd, sizeof(cmd), fid, vid)) {
            DEBUG("No more temperature record to read--ForwarderID=0x%08X, ValveID=0x%04X\n", fid, vid);
            return counter;
        }
        counter ++;
    }

    return counter;
}

uint32 CForwarderMonitor::BatchGetRecordsData()
{
    DEBUG("\n");
    uint16 counter = 0;
    for (ForwarderMapT::iterator ForwarderIter = m_DraftForwarderMap.begin(); ForwarderIter != m_DraftForwarderMap.end(); ForwarderIter++ ) {
        if (ForwarderIter->second.IsOffline) {
            DEBUG("0x%8x offline\n", ForwarderIter->first);
            continue;
        }
        for (ValveListT::iterator ValveIter = ForwarderIter->second.ValveList.begin(); ValveIter != ForwarderIter->second.ValveList.end(); ValveIter++) {
            if (false == ValveIter->second.IsActive) {
                DEBUG("ForwarderID(0x%8X) ValveID(%04X) offline\n", ForwarderIter->first, ValveIter->first);
                continue;
            }

            uint32 vmac = ((ForwarderIter->first & 0xFFFF) << 16) | ValveIter->first;
            record_t rec = records[vmac];

            // Get consume data
            counter += BatchGetConsumeRecords(ForwarderIter->first, ValveIter->first);

            // Get charge data
            counter += BatchGetRechargeRecords(ForwarderIter->first, ValveIter->first);

            if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
                // Get temperature data
                counter += BatchGetTemperatureRecords(ForwarderIter->first, ValveIter->first);
            }
        }
    }
    return counter;
}
