#include<assert.h>
#include"Utils.h"
#include"CHeatMonitor.h"
#include"CPortal.h"

#ifdef DEBUG_HEAT
#include <time.h>
#define DEBUG(...) do {printf("%ld %s::%s %d ----", time(NULL), __FILE__, __func__, __LINE__);printf(__VA_ARGS__);} while(false)
#ifndef hexdump
#define hexdump(data, len) do {for (uint32 i = 0; i < (uint32)len; i ++) { printf("%02x ", *(uint8 *)(data + i));} printf("\n");} while(0)
#endif
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
#endif

const int32 HEAT_TIMEOUT =  10*1000*1000;//10 second
const uint32 GENERAL_HEAT_COMMAND_ACK_LEN = 253;
const uint32 HEATINFO_TIMEOUT = 300;//s
const uint8 ERROR_FLAG[] = {0xBD, 0xEB};

CLock CHeatMonitor::m_HeatLock;
CHeatMonitor* CHeatMonitor::m_Instance = NULL;
CHeatMonitor* CHeatMonitor::GetInstance()
{
    if ( NULL == m_Instance ) {
        m_HeatLock.Lock();
        if ( NULL == m_Instance ) {
            m_Instance = new CHeatMonitor();
        }
        m_HeatLock.UnLock();
    }
    return m_Instance;
}

bool CHeatMonitor::Init(uint32 nStartTime, uint32 nInterval)
{
    return m_RepeatTimer.Start( nInterval*60 );/*unit:s*/
}

void CHeatMonitor::AddGeneralHeat(uint8* pGeneralHeatMacAddress, uint32 Len)
{
    if ( NULL == pGeneralHeatMacAddress || Len <= 0 || Len > MACHINENAME_LEN ) {
        return;
    }

    HeatNodeDataT HeatNodeData;
    memset(&HeatNodeData, 0, sizeof(HeatNodeData));
    HeatNodeData.IsOffline = true;
    memcpy(HeatNodeData.MacAddress, pGeneralHeatMacAddress, Len);
    m_HeatNodeVector.push_back(HeatNodeData);
}

uint32 CHeatMonitor::Run()
{
    while (1) {

        m_HeatLock.Lock();
        if ( m_RepeatTimer.Done() ) {
            GetHeatData();
        }
        m_HeatLock.UnLock();

        m_HeatLock.Lock();
        GetHeatInfoTask();
        m_HeatLock.UnLock();

        sleep(1);
    }
}

void CHeatMonitor::GetHeatData()
{
    const uint8 COMMAND_HEAT_NODE_ADDRESS_INDEX = 2;
    const uint8 COMMAND_CHECKSUM_INDEX = 3;
    uint8 SendData[] = {0x10, 0x5B, 0xFE/*heat node address*/, 0x59/*check sum*/, 0x16};//Boardcast heat
    for (HeatNodeVectorT::iterator iter = m_HeatNodeVector.begin(); iter != m_HeatNodeVector.end(); iter++) {
        SendData[COMMAND_HEAT_NODE_ADDRESS_INDEX] = iter->MacAddress[0];
        SendData[COMMAND_CHECKSUM_INDEX] = GetCheckSum(SendData+1, COMMAND_HEAT_NODE_ADDRESS_INDEX);
        DEBUG("Write to heat ");
        hexdump(SendData, sizeof(SendData));

        uint32 SendDataLen = sizeof(SendData);
        FlashLight(LIGHT_GENERAL_HEAT);
        if (!SendCommand(SendData, SendDataLen)) {
            DEBUG("WriteError\n");
            return;
        }
        uint8 buf[256];
        bzero(buf, 256);
        uint16 len = 256;
        FlashLight(LIGHT_GENERAL_HEAT);
        if (WaitCmdAck(buf, &len)) {
            DEBUG("Read from heat ");
            hexdump(buf, len);
            if (ParseData(buf, len, iter)) {
                iter->IsOffline = false;
                continue;
            }
        } else {
            DEBUG("Read from heat error\n");
        }
    }

    //heat info is ready
    m_IsHeatInfoReady = true;
    m_HeatInfoTimeOut.StartTimer(HEATINFO_TIMEOUT);
}

bool CHeatMonitor::ParseData(const uint8* pData, uint32 DataLen, HeatNodeVectorT::iterator iter)
{
    assert(iter != m_HeatNodeVector.end());
    if (iter == m_HeatNodeVector.end()) {
        DEBUG("Invalid node iterator\n");
        return false;
    }

    const uint32 SUPPLY_WATER_TEMPERATURE_INDEX = 45; // 0x38; // 56
    memcpy(iter->SupplyWaterTemperature, pData+SUPPLY_WATER_TEMPERATURE_INDEX, sizeof(iter->SupplyWaterTemperature));
    DEBUG("SupplyWaterTemperature ");
    hexdump(iter->SupplyWaterTemperature, GENERALHEAT_TEMPERATURE_LEN);
    iter->SupplyWaterTemperatureDIF = pData[SUPPLY_WATER_TEMPERATURE_INDEX - 2];

    const uint32 RETURN_WATER_TEMPERATURE_INDEX = 51; // 0x3C; // 60
    memcpy(iter->ReturnWaterTemperature, pData+RETURN_WATER_TEMPERATURE_INDEX, sizeof(iter->ReturnWaterTemperature));
    DEBUG("ReturnWaterTemperature ");
    hexdump(iter->ReturnWaterTemperature, GENERALHEAT_TEMPERATURE_LEN);
    iter->ReturnWaterTemperatureDIF = pData[RETURN_WATER_TEMPERATURE_INDEX - 2];

    const uint32 CURRENT_FLOW_VELOCITY_INDEX = 75; // 0x35; // 53
    memcpy(iter->CurrentFlowVelocity, pData+CURRENT_FLOW_VELOCITY_INDEX , sizeof(iter->CurrentFlowVelocity)-1);//current flow only has three bytes
    DEBUG("CurrentFlowVelocity ");
    hexdump(iter->CurrentFlowVelocity, VELOCITY_LEN);
    iter->CurrentFlowVelocityDIF = pData[CURRENT_FLOW_VELOCITY_INDEX - 2];
    // memcpy(iter->CurrentFlowVelocity, pData+CURRENT_FLOW_VELOCITY_INDEX , sizeof(iter->CurrentFlowVelocity));

    const uint32 CURRENT_HEAT_VELOCITY_INDEX = 63;// 0x2F; // 47
    memcpy(iter->CurrentHeatVelocity, pData+CURRENT_HEAT_VELOCITY_INDEX, sizeof(iter->CurrentHeatVelocity));
    DEBUG("CurrentHeatVelocity ");
    hexdump(iter->CurrentHeatVelocity, VELOCITY_LEN);
    iter->CurrentHeatVelocityDIF = pData[CURRENT_HEAT_VELOCITY_INDEX - 2];

    const uint32 TOTAL_FLOW_CAPACITY_INDEX = 33; // 0x28; // 40
    memcpy(iter->TotalFlow, pData+TOTAL_FLOW_CAPACITY_INDEX, sizeof(iter->TotalFlow));
    DEBUG("TotalFlow ");
    hexdump(iter->TotalFlow, CAPACITY_LEN);
    iter->TotalFlowDIF = pData[TOTAL_FLOW_CAPACITY_INDEX - 2];

    const uint32 TOTAL_HEAT_CAPACITY_INDEX = 27; // 0x14; // 20
    memcpy(iter->TotalHeat, pData+TOTAL_HEAT_CAPACITY_INDEX, sizeof(iter->TotalHeat));
    DEBUG("TotalHeat ");
    hexdump(iter->TotalHeat, CAPACITY_LEN);
    iter->TotalHeatDIF = pData[TOTAL_HEAT_CAPACITY_INDEX - 2];

    // const uint32 TOTAL_WORK_HOURS_INDEX = 39;
    // memcpy(iter->TotalWorkHours, pData+TOTAL_WORK_HOURS_INDEX, sizeof(iter->TotalWorkHours));
    memset(iter->TotalWorkHours, 0, sizeof(iter->TotalWorkHours));//don't need work hours

    // const uint32 UTCTIME_INDEX = 124;
    // memcpy(&(iter->CurrentTime), pData+UTCTIME_INDEX,  sizeof(iter->CurrentTime));
    memset(&(iter->CurrentTime), 0, sizeof(iter->CurrentTime));

    return true;
}

void CHeatMonitor::SendHeatData()
{
    if (m_HeatNodeVector.size() > 0) {
        uint8 data[1024];
        uint8 ptr = 2; // skip node count
        uint16 count = 0;
        bzero(data, 1024);
        m_HeatLock.Lock();
        for (HeatNodeVectorT::iterator iter = m_HeatNodeVector.begin(); iter != m_HeatNodeVector.end(); iter++) {
            if (iter->IsOffline) {
                continue;
            }
            count ++;

            data[ptr] = 0xF1; ptr ++;
            data[ptr] = 35; ptr ++; // data len

            DEBUG("HeatData: ");
            hexdump(iter->MacAddress, sizeof(HeatNodeDataT));
            memcpy(data + ptr, iter->MacAddress + 2, sizeof(iter->MacAddress) - 2); ptr += sizeof(iter->MacAddress) - 2;
            data[ptr] = iter->MacAddress[1]; ptr ++;
            memcpy(data + ptr, iter->SupplyWaterTemperature, GENERALHEAT_TEMPERATURE_LEN); ptr += GENERALHEAT_TEMPERATURE_LEN;
            data[ptr] = iter->SupplyWaterTemperatureDIF; ptr ++; // error code?
            memcpy(data + ptr, iter->ReturnWaterTemperature, GENERALHEAT_TEMPERATURE_LEN); ptr += GENERALHEAT_TEMPERATURE_LEN;
            data[ptr] = iter->ReturnWaterTemperatureDIF; ptr ++; // error code?
            memcpy(data + ptr, iter->CurrentFlowVelocity, VELOCITY_LEN); ptr += VELOCITY_LEN;
            data[ptr] = iter->CurrentFlowVelocityDIF; ptr ++; // error code?

            memcpy(data + ptr, iter->CurrentHeatVelocity, VELOCITY_LEN); ptr += VELOCITY_LEN;
            data[ptr] = iter->CurrentHeatVelocityDIF; ptr ++; // error code?
            memcpy(data + ptr, iter->TotalFlow, CAPACITY_LEN); ptr += CAPACITY_LEN;
            data[ptr] = iter->TotalFlowDIF; ptr ++; // error code?
            memcpy(data + ptr, iter->TotalHeat, CAPACITY_LEN); ptr += CAPACITY_LEN;
            data[ptr] = iter->TotalHeatDIF; ptr ++; // error code?
        }
        if (ptr > 2) {
            DEBUG("before setting count ");
            hexdump(data, ptr);
            DEBUG("count is %d\n", count);
            memcpy(data, &count, 2); // set node count
            CPortal::GetInstance()->InsertGeneralHeatData(data, ptr);
        }
    }
    m_HeatLock.UnLock();
}

bool CHeatMonitor::GetStatus(Status& Status)
{
    DEBUG("CHeatMonitor::GetStatus()\n");
    bool Ret = false;
    if ( m_HeatLock.TryLock() ) {
        if ( false == IsStarted() ) {
            DEBUG("Status error\n");
            Status = STATUS_ERROR;
            Ret = true;
        } else {
            if ( m_IsHeatInfoReady ) {
                Status = STATUS_OFFLINE;
                for (uint32 i = 0; i < m_HeatNodeVector.size(); i++) {
                    if (false == m_HeatNodeVector[i].IsOffline) {
                        Status = STATUS_OK;
                        break;
                    }
                }

                Ret = true;
            } else {
                m_GetHeatInforTaskActive = true;
            }
        }

        m_HeatLock.UnLock();
    }

    return Ret;
}

bool CHeatMonitor::GetHeatNodeInfoList(HeatNodeInfoListT& HeatNodeInfoList)
{
    bool Ret = false;
    if (m_HeatLock.TryLock()) {
        if ( m_IsHeatInfoReady ) {
            HeatNodeInfoList.clear();
            for (uint32 i = 0; i < m_HeatNodeVector.size(); i++) {
                DEBUG("i=%d\n", i);
                HeatNodeInfoT HeatNodeInfo;

                HeatNodeInfo.IsOffline = m_HeatNodeVector[i].IsOffline;
                //MacAddress
                memcpy(HeatNodeInfo.MacAddress, m_HeatNodeVector[i].MacAddress, sizeof(m_HeatNodeVector[i].MacAddress));

                //SupplyWaterTemperature
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].SupplyWaterTemperature, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("SupplyWaterTemperature\n");
                    hexdump(m_HeatNodeVector[i].SupplyWaterTemperature, sizeof(m_HeatNodeVector[i].SupplyWaterTemperature));
                    memset( HeatNodeInfo.SupplyWaterTemperature, 0, sizeof(HeatNodeInfo.SupplyWaterTemperature) );
                    uint32 SupplyWaterTemperature= 10*((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-1]>>4)&0x0F)
                                                   + ((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-1]&0x0F));
                    SupplyWaterTemperature = 100*SupplyWaterTemperature
                                             + 10*((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-2]>>4)&0x0F)
                                             + (m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-2]&0x0F);
                    DEBUG("SupplyWaterTemperature=%u\n", SupplyWaterTemperature);
                    sprintf( HeatNodeInfo.SupplyWaterTemperature, "%d.%d", SupplyWaterTemperature/10, SupplyWaterTemperature%10 );
                } else {
                    sprintf( HeatNodeInfo.SupplyWaterTemperature, "Err" );
                }
                //ReturnWaterTemperature
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].ReturnWaterTemperature, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("ReturnWaterTemperature\n");
                    hexdump(m_HeatNodeVector[i].ReturnWaterTemperature, sizeof(m_HeatNodeVector[i].ReturnWaterTemperature));
                    memset( HeatNodeInfo.ReturnWaterTemperature, 0, sizeof(HeatNodeInfo.ReturnWaterTemperature) );
                    uint32 ReturnWaterTemperature= 10*((m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-1]>>4)&0x0F)
                                                   + (m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-1]&0x0F);
                    ReturnWaterTemperature = 100*ReturnWaterTemperature
                                             + 10*((m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-2]>>4)&0x0F)
                                             + (m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-2]&0x0F);
                    sprintf( HeatNodeInfo.ReturnWaterTemperature, "%d.%d", ReturnWaterTemperature/10, ReturnWaterTemperature%10 );
                } else {
                    sprintf( HeatNodeInfo.ReturnWaterTemperature , "Err" );
                }
                //CurrentFlowVelocity
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].CurrentFlowVelocity, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("CurrentFlowVelocity\n");
                    hexdump(m_HeatNodeVector[i].CurrentFlowVelocity, sizeof(m_HeatNodeVector[i].CurrentFlowVelocity));
                    memset( HeatNodeInfo.CurrentFlowVelocity, 0, sizeof(HeatNodeInfo.CurrentFlowVelocity) );
                    uint32 CurrentFlowVelocity= 10*((m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-1]>>4)&0x0F)
                                                + (m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-1]&0x0F);
                    CurrentFlowVelocity = 100*CurrentFlowVelocity
                                          + 10*((m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-2]>>4)&0x0F)
                                          + (m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-2]&0x0F);
                    CurrentFlowVelocity = 100*CurrentFlowVelocity
                                          + 10*((m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-3]>>4)&0x0F)
                                          + (m_HeatNodeVector[i].CurrentFlowVelocity[sizeof(m_HeatNodeVector[i].CurrentFlowVelocity)-3]&0x0F);
                    sprintf( HeatNodeInfo.CurrentFlowVelocity, "%d.%d", CurrentFlowVelocity/1000, CurrentFlowVelocity%1000 );
                } else {
                    sprintf( HeatNodeInfo.CurrentFlowVelocity, "Err" );
                }
                //CurrentHeatVelocity
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].CurrentHeatVelocity, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("CurrentHeatVelocity\n");
                    hexdump(m_HeatNodeVector[i].CurrentHeatVelocity, sizeof(m_HeatNodeVector[i].CurrentHeatVelocity));
                    memset( HeatNodeInfo.CurrentHeatVelocity, 0, sizeof(HeatNodeInfo.CurrentHeatVelocity) );
                    uint32 CurrentHeatVelocity= 10*((m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-1]>>4)&0x0F)
                                                + (m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-1]&0x0F);
                    CurrentHeatVelocity = 100*CurrentHeatVelocity
                                          + 10*((m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-2]>>4)&0x0F)
                                          + (m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-2]&0x0F);
                    CurrentHeatVelocity = 100*CurrentHeatVelocity
                                          + 10*((m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-3]>>4)&0x0F)
                                          + (m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-3]&0x0F);
                    CurrentHeatVelocity = 100*CurrentHeatVelocity
                                          + 10*((m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-4]>>4)&0x0F)
                                          + (m_HeatNodeVector[i].CurrentHeatVelocity[sizeof(m_HeatNodeVector[i].CurrentHeatVelocity)-4]&0x0F);
                    sprintf( HeatNodeInfo.CurrentHeatVelocity, "%d.%d", CurrentHeatVelocity/1000, CurrentHeatVelocity%1000 );
                } else {
                    sprintf( HeatNodeInfo.CurrentHeatVelocity, "Err" );
                }
                //TotalFlow
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].TotalFlow, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("TotalFlow\n");
                    hexdump(m_HeatNodeVector[i].TotalFlow, sizeof(m_HeatNodeVector[i].TotalFlow));
                    memset( HeatNodeInfo.TotalFlow, 0, sizeof(HeatNodeInfo.TotalFlow) );
                    uint32 TotalFlow= 10*((m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-1]>>4)&0x0F)
                                      + (m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-1]&0x0F);
                    TotalFlow = 100*TotalFlow
                                + 10*((m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-2]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-2]&0x0F);
                    TotalFlow = 100*TotalFlow
                                + 10*((m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-3]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-3]&0x0F);
                    TotalFlow = 100*TotalFlow
                                + 10*((m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-4]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalFlow[sizeof(m_HeatNodeVector[i].TotalFlow)-4]&0x0F);
                    sprintf( HeatNodeInfo.TotalFlow, "%d.%d", TotalFlow/100, TotalFlow%100 );
                } else {
                    sprintf( HeatNodeInfo.TotalFlow, "Err" );
                }
                //TotalHeat
                if ( memcmp( ERROR_FLAG, m_HeatNodeVector[i].TotalHeat, sizeof(ERROR_FLAG) ) ) {
                    DEBUG("TotalHeat\n");
                    hexdump(m_HeatNodeVector[i].TotalHeat, sizeof(m_HeatNodeVector[i].TotalHeat));
                    memset( HeatNodeInfo.TotalHeat, 0, sizeof(HeatNodeInfo.TotalHeat) );
                    uint32 TotalHeat= 10*((m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-1]>>4)&0x0F)
                                      + (m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-1]&0x0F);
                    TotalHeat = 100*TotalHeat
                                + 10*((m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-2]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-2]&0x0F);
                    TotalHeat = 100*TotalHeat
                                + 10*((m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-3]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-3]&0x0F);
                    TotalHeat = 100*TotalHeat
                                + 10*((m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-4]>>4)&0x0F)
                                + (m_HeatNodeVector[i].TotalHeat[sizeof(m_HeatNodeVector[i].TotalHeat)-4]&0x0F);
                    sprintf( HeatNodeInfo.TotalHeat, "%d.%d", TotalHeat/100, TotalHeat%100 );
                } else {
                    sprintf( HeatNodeInfo.TotalHeat, "Err" );
                }

                HeatNodeInfoList.push_back(HeatNodeInfo);
            }

            Ret = true;
        } else {
            m_GetHeatInforTaskActive = true;
        }
        m_HeatLock.UnLock();
    }

    return Ret;
}

void CHeatMonitor::GetHeatInfoTask()
{
    //DEBUG("m_GetHeatInforTaskActive=%d, m_IsHeatInfoReady=%d\n", m_GetHeatInforTaskActive, m_IsHeatInfoReady);
    if ( m_GetHeatInforTaskActive ) {
        if ( false == m_IsHeatInfoReady ) {
            DEBUG("GetHeatInfo\n");
            GetHeatData();
        }

        if ( m_IsHeatInfoReady ) {
            m_GetHeatInforTaskActive = false;
        }
    }

    if ( m_HeatInfoTimeOut.Done() ) {
        DEBUG("HeatInfo NOT ready\n");
        m_IsHeatInfoReady = false;
    }
}

bool CHeatMonitor::SendCommand(uint8 * data, uint16 len)
{
    fd_set wfds;
    struct timeval tv;
    int retval, w, wrote = 0;
    uint8 * buf = data;
    uint16 wlen = len;
    uint8 retry = 3;

    FD_ZERO(&wfds);
    FD_SET(com, &wfds);

    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (retry > 0) {
        retry --;
        retval = select(com + 1, NULL, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &wfds)) {
                w = write(com, buf + wrote, wlen - wrote);
                if (w == -1) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    DEBUG("Write %d bytes:", wlen);
                    hexdump(buf, wlen);
                    wrote = 0;
                    myusleep(500 * 1000);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CHeatMonitor::WaitCmdAck(uint8 * data, uint16 * len)
{
    fd_set rfds;
    struct timeval tv;
    int retval, r, readed = 0;
    uint8 * ack = data;
    uint16 rlen = 0;
    uint8 retry = 3;

    FD_ZERO(&rfds);
    FD_SET(com, &rfds);

    while (retry > 0) {
        retry --;
        tv.tv_sec = 1;
        tv.tv_usec = 0;
        retval = select(com + 1, &rfds, NULL, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = 253;
                    bzero(ack, 256);
                }

                r = read(com, ack + readed, rlen - readed);

                if (r == -1) {
                    continue;
                }
                readed += r;

                if (readed == rlen) {
                    readed = 0;
                    DEBUG("read %d bytes: ", rlen);
                    hexdump(ack, rlen);
                    * len = rlen;
                    return true;
                }
            }
        }
    }
    return false;
}
