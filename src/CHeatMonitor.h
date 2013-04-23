#ifndef __CHEATMONITOR_H
#define __CHEATMONITOR_H
#include<vector>
#include"IThread.h"
#include"CTimer.h"
#include"CSerialComm.h"
#include"CLock.h"
#include"sysdefs.h"
using namespace std;
const uint8 MACHINENAME_BEGIN = 2;
const uint8 MACHINENAME_LEN = 10;
const uint8 VELOCITY_LEN = 4;
const uint8 CAPACITY_LEN = 5;
const uint8 GENERALHEAT_TEMPERATURE_LEN = 4;
const uint8 TOTAL_WORKHOURS_LEN = 4;
const uint8 GENERALHEAT_TIME_LEN = 7;
enum HeatNodeT
{
   HEAT_NODE_HEAT=0,
   HEAT_NODE_COLD,
   HEAT_NODE_TYPE_MAX//never delete this
};
struct HeatNodeDataT
{
   bool IsOffline;
   uint8 MacAddress[MACHINENAME_LEN];
   uint8 SupplyWaterTemperature[GENERALHEAT_TEMPERATURE_LEN];
   uint8 ReturnWaterTemperature[GENERALHEAT_TEMPERATURE_LEN];
   uint8 CurrentFlowVelocity[VELOCITY_LEN];
   uint8 CurrentHeatVelocity[VELOCITY_LEN];
   uint8 TotalFlow[CAPACITY_LEN];
   uint8 TotalHeat[CAPACITY_LEN];
   uint8 TotalWorkHours[TOTAL_WORKHOURS_LEN];
   uint32 CurrentTime;
}__attribute__( ( packed, aligned(1) ) );
typedef vector<HeatNodeDataT> HeatNodeVectorT;
const uint32 HEAT_NODE_DATA_MAX_LEN = 32;
struct HeatNodeInfoT
{
   bool IsOffline;
   uint8 MacAddress[MACHINENAME_LEN];
   char SupplyWaterTemperature[HEAT_NODE_DATA_MAX_LEN];
   char ReturnWaterTemperature[HEAT_NODE_DATA_MAX_LEN];
   char CurrentFlowVelocity[HEAT_NODE_DATA_MAX_LEN];
   char CurrentHeatVelocity[HEAT_NODE_DATA_MAX_LEN];
   char TotalFlow[HEAT_NODE_DATA_MAX_LEN];
   char TotalHeat[HEAT_NODE_DATA_MAX_LEN];
};
typedef vector<HeatNodeInfoT> HeatNodeInfoListT;

const uint8 DEFAULT_SAMPLECOUNTPERHOUR = 4;
class CHeatMonitor:public IThread

{
  public:
    void SetCom(CSerialComm* pCom);
    bool Init(uint32 nStartTime, uint32 nInterval);
    static CHeatMonitor* GetInstance();
  private:
    static CHeatMonitor* m_Instance;
    static CLock m_HeatLock;
    CHeatMonitor():m_pCommController(NULL), m_GetHeatInforTaskActive(false), m_IsHeatInfoReady(false){}

  public:
    void AddGeneralHeat(uint8* pGeneralHeatMacAddress, uint32 Len);
    void SendHeatData();
    bool GetStatus(Status& Status);
    bool GetHeatNodeInfoList(HeatNodeInfoListT& HeatNodeInfoList);

  private:
    virtual uint32 Run();

  private:
    CRepeatTimer m_RepeatTimer;

  private:
    void GetHeatData();
    bool ParseData(const uint8* pData, uint32 DataLen, HeatNodeVectorT::iterator HeatNodeIter);
  protected:
    CSerialComm* m_pCommController;
    HeatNodeVectorT m_HeatNodeVector;

  private:
    void GetHeatInfoTask();
  private:
    bool m_GetHeatInforTaskActive;
    bool m_IsHeatInfoReady;
    CTimer m_HeatInfoTimeOut;
};
#endif
