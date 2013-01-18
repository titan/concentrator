#include<assert.h>
#include"Utils.h"
#include"CHeatMonitor.h"
#include"CPortal.h"

const int32 HEAT_TIMEOUT =  10*1000*1000;//10 second
const uint32 GENERAL_HEAT_COMMAND_ACK_LEN = 154;
const uint32 HEATINFO_TIMEOUT = 300;//s
const uint8 ERROR_FLAG[] = {0xBD, 0xEB};

CLock CHeatMonitor::m_HeatLock;
CHeatMonitor* CHeatMonitor::m_Instance = NULL;
CHeatMonitor* CHeatMonitor::GetInstance()
{
   if( NULL == m_Instance )
   {
      m_HeatLock.Lock();
      if( NULL == m_Instance )
      {
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

void CHeatMonitor::SetCom(CSerialComm* pCom)
{
   if( (NULL == pCom) || (m_pCommController == pCom) )
   {
      return;
   }

   m_pCommController = pCom;
}

void CHeatMonitor::AddGeneralHeat(uint8* pGeneralHeatMacAddress, uint32 Len)
{
   if( NULL == pGeneralHeatMacAddress || Len <= 0 || Len > MACHINENAME_LEN )
   {
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
   while(1)
   {
      printf("CHeatMonitor::Run()\n");

      m_HeatLock.Lock();
      if( m_RepeatTimer.Done() )
      {
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
   if(NULL == m_pCommController)
   {
      printf("CHeatMonitor::GetHeatData()--COMM is NULL\n");
      return;
   }

   const uint8 COMMAND_HEAT_NODE_ADDRESS_INDEX = 2;
   const uint8 COMMAND_CHECKSUM_INDEX = 3;
   uint8 SendData[] = {0x10, 0x5B, 0xFE/*heat node address*/, 0x59/*check sum*/, 0x16};//Boardcast heat
   for(HeatNodeVectorT::iterator HeatNodeIter = m_HeatNodeVector.begin(); HeatNodeIter != m_HeatNodeVector.end(); HeatNodeIter++)
   {
      //right now only one heat meter attached, use Boardcast message to get data
      //so comment the following data
      if( (HeatNodeIter->MacAddress[0]+10*HeatNodeIter->MacAddress[1]) >= 0xFF )
      {
         continue;
      }
      uint8 MacAddress = HeatNodeIter->MacAddress[0]+10*HeatNodeIter->MacAddress[1];
      SendData[COMMAND_HEAT_NODE_ADDRESS_INDEX] = HeatNodeIter->MacAddress[0];
      SendData[COMMAND_CHECKSUM_INDEX] = GetCheckSum(SendData+1, COMMAND_HEAT_NODE_ADDRESS_INDEX);
      PrintData(SendData, sizeof(SendData));

      uint32 SendDataLen = sizeof(SendData);
      FlashLight(LIGHT_GENERAL_HEAT);
      if(COMM_OK != m_pCommController->WriteBuf(SendData, SendDataLen, HEAT_TIMEOUT))
      {
         printf("CHeatMonitor::GetHeatData()----WriteError\n");
         return;
      }

      uint8 Buffer[MAX_BUFFER_LEN] = {0};
      uint32 BufferCount = sizeof(Buffer);
      FlashLight(LIGHT_GENERAL_HEAT);
      if(COMM_OK == m_pCommController->ReadMinByte(Buffer, BufferCount, GENERAL_HEAT_COMMAND_ACK_LEN, 0x68, 0x16, HEAT_TIMEOUT) )
      {
         PrintData(Buffer, BufferCount);
         if(ParseData(Buffer, BufferCount, HeatNodeIter))
         {
            HeatNodeIter->IsOffline = false;
            continue;
         }
      }
   }

   //heat info is ready
   m_IsHeatInfoReady = true;
   m_HeatInfoTimeOut.StartTimer(HEATINFO_TIMEOUT);
}

bool CHeatMonitor::ParseData(const uint8* pData, uint32 DataLen, HeatNodeVectorT::iterator HeatNodeIter)
{
   if(GENERAL_HEAT_COMMAND_ACK_LEN != DataLen)
   {
      printf("CHeatMonitor::ParseData()----DataLen(%d) NOT match\n", DataLen);
      assert(GENERAL_HEAT_COMMAND_ACK_LEN == DataLen);
      return false;
   }
   assert(HeatNodeIter != m_HeatNodeVector.end());
   if(HeatNodeIter == m_HeatNodeVector.end())
   {
      printf("CHeatMonitor::ParseData()----Invalid node iterator\n");
      return false;
   }


   const uint32 SUPPLY_WATER_TEMPERATURE_INDEX = 0x38;
   memcpy(HeatNodeIter->SupplyWaterTemperature, pData+SUPPLY_WATER_TEMPERATURE_INDEX, sizeof(HeatNodeIter->SupplyWaterTemperature));

   const uint32 RETURN_WATER_TEMPERATURE_INDEX = 0x3C;
   memcpy(HeatNodeIter->ReturnWaterTemperature, pData+RETURN_WATER_TEMPERATURE_INDEX, sizeof(HeatNodeIter->ReturnWaterTemperature));

   const uint32 CURRENT_FLOW_VELOCITY_INDEX = 0x35;
   memcpy(HeatNodeIter->CurrentFlowVelocity, pData+CURRENT_FLOW_VELOCITY_INDEX , sizeof(HeatNodeIter->CurrentFlowVelocity)-1);//current flow only has three bytes
   // memcpy(HeatNodeIter->CurrentFlowVelocity, pData+CURRENT_FLOW_VELOCITY_INDEX , sizeof(HeatNodeIter->CurrentFlowVelocity));

   const uint32 CURRENT_HEAT_VELOCITY_INDEX = 0x2F;
   memcpy(HeatNodeIter->CurrentHeatVelocity, pData+CURRENT_HEAT_VELOCITY_INDEX, sizeof(HeatNodeIter->CurrentHeatVelocity));

   const uint32 TOTAL_FLOW_CAPACITY_INDEX = 0x28;
   memcpy(HeatNodeIter->TotalFlow, pData+TOTAL_FLOW_CAPACITY_INDEX, sizeof(HeatNodeIter->TotalFlow));

   const uint32 TOTAL_HEAT_CAPACITY_INDEX = 0x14;
   memcpy(HeatNodeIter->TotalHeat, pData+TOTAL_HEAT_CAPACITY_INDEX, sizeof(HeatNodeIter->TotalHeat));

   // const uint32 TOTAL_WORK_HOURS_INDEX = 39;
   // memcpy(HeatNodeIter->TotalWorkHours, pData+TOTAL_WORK_HOURS_INDEX, sizeof(HeatNodeIter->TotalWorkHours));
   memset(HeatNodeIter->TotalWorkHours, 0, sizeof(HeatNodeIter->TotalWorkHours));//don't need work hours

   // const uint32 UTCTIME_INDEX = 124;
   // memcpy(&(HeatNodeIter->CurrentTime), pData+UTCTIME_INDEX,  sizeof(HeatNodeIter->CurrentTime));
   memset(&(HeatNodeIter->CurrentTime), 0, sizeof(HeatNodeIter->CurrentTime));

   return true;
}

void CHeatMonitor::SendHeatData()
{
   const uint8 GeneralLeaderCharacter = 0xF1;
   const uint8 GeneralHeatLen = GENERAL_HEAT_DATA_PACKET_LEN-sizeof(GeneralLeaderCharacter)-sizeof(GeneralHeatLen);
   uint8 GeneralHeatData[GENERAL_HEAT_DATA_PACKET_LEN] = {0};
   GeneralHeatData[0] = GeneralLeaderCharacter;
   GeneralHeatData[sizeof(GeneralLeaderCharacter)] = GeneralHeatLen;
   m_HeatLock.Lock();
   if(m_HeatNodeVector.size() > 0)
   {
      HeatNodeVectorT::iterator Iter = m_HeatNodeVector.begin();
      for(; Iter != m_HeatNodeVector.end(); Iter++)
      {
         if(Iter->IsOffline)
         {
            continue;
         }
         printf("CHeatMonitor::SendHeatData()----HeatData:");
         PrintData(Iter->MacAddress, sizeof(HeatNodeDataT));
         memcpy(GeneralHeatData+sizeof(GeneralLeaderCharacter)+sizeof(GeneralHeatLen), Iter->MacAddress, GeneralHeatLen);

         //adjust the data
         //adjust the SupplyWaterTemperature
         const uint8 SUPPLY_WATERT_EMPERATURE_POS = sizeof(GeneralLeaderCharacter)+sizeof(GeneralHeatLen)+MACHINENAME_LEN;
         uint32 SupplyWaterTemperature = 0;
         PrintData(GeneralHeatData+SUPPLY_WATERT_EMPERATURE_POS, sizeof(ERROR_FLAG));
         if( memcmp(ERROR_FLAG, GeneralHeatData+SUPPLY_WATERT_EMPERATURE_POS, sizeof(ERROR_FLAG)) )
         {
            SupplyWaterTemperature = 10*((GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-1]>>4)&0x0F)
                                     + ((GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-1]&0x0F));
            SupplyWaterTemperature = 100*SupplyWaterTemperature
                                     + 10*((GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-2]>>4)&0x0F)
                                     + (GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-2]&0x0F);
         }
         GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS] = 0x01;//unit

         printf("CHeatMonitor::SendHeatData()---SupplyWaterTemperature=%u\n", SupplyWaterTemperature);
         GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+1] = (SupplyWaterTemperature%10)<<4;
         SupplyWaterTemperature /= 10;
         GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+2] = (((SupplyWaterTemperature%100)/10)<<4)|((SupplyWaterTemperature%100)%10);
         SupplyWaterTemperature /= 100;
         GeneralHeatData[SUPPLY_WATERT_EMPERATURE_POS+3] = (SupplyWaterTemperature%100)<<4;
         //adjust the ReturnWaterTemperature
         const uint8 RETURN_WATERT_EMPERATURE_POS = SUPPLY_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN;
         uint32 ReturnWaterTemperature = 0;
         PrintData(GeneralHeatData+RETURN_WATERT_EMPERATURE_POS, sizeof(ERROR_FLAG));
         if( memcmp(ERROR_FLAG, GeneralHeatData+RETURN_WATERT_EMPERATURE_POS, sizeof(ERROR_FLAG)) )
         {
            ReturnWaterTemperature = 10*((GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-1]>>4)&0x0F)
                                     + ((GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-1]&0x0F));
            ReturnWaterTemperature = 100*ReturnWaterTemperature
                                     + 10*((GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-2]>>4)&0x0F)
                                     + (GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN-2]&0x0F);
         }
         GeneralHeatData[RETURN_WATERT_EMPERATURE_POS] = 0x01;//unit

         printf("CHeatMonitor::SendHeatData()---ReturnWaterTemperature=%u\n", ReturnWaterTemperature);
         GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+1] = (ReturnWaterTemperature%10)<<4;
         ReturnWaterTemperature /= 10;
         GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+2] = (((ReturnWaterTemperature%100)/10)<<4)|((ReturnWaterTemperature%100)%10);
         ReturnWaterTemperature /= 100;
         GeneralHeatData[RETURN_WATERT_EMPERATURE_POS+3] = (ReturnWaterTemperature%100)<<4;
         //adjust CurrentFlowVelocity
         const uint8 CURRENT_FLOW_VELOCITY_POS = RETURN_WATERT_EMPERATURE_POS+GENERALHEAT_TEMPERATURE_LEN;
         PrintData(GeneralHeatData+CURRENT_FLOW_VELOCITY_POS, sizeof(ERROR_FLAG));
         if( 0 == memcmp(ERROR_FLAG, GeneralHeatData+CURRENT_FLOW_VELOCITY_POS, sizeof(ERROR_FLAG)) )
         {
            memset(GeneralHeatData+CURRENT_FLOW_VELOCITY_POS+1, 0, VELOCITY_LEN-1);
         }
         GeneralHeatData[CURRENT_FLOW_VELOCITY_POS] = 0x01;//unit
         //adjust CurrentHeatVelocity
         const uint8 CURRENT_HEAT_VELOCITY_POS = CURRENT_FLOW_VELOCITY_POS+VELOCITY_LEN;
         PrintData(GeneralHeatData+CURRENT_HEAT_VELOCITY_POS, sizeof(ERROR_FLAG));
         if( 0 == memcmp(ERROR_FLAG, GeneralHeatData+CURRENT_HEAT_VELOCITY_POS, sizeof(ERROR_FLAG)) )
         {
            memset(GeneralHeatData+CURRENT_HEAT_VELOCITY_POS+1, 0, VELOCITY_LEN-1);
         }
         GeneralHeatData[CURRENT_HEAT_VELOCITY_POS] = 0x01;//unit
         //adjust TotalFlow
         const uint8 TOTAL_FLOW_POS = CURRENT_HEAT_VELOCITY_POS+VELOCITY_LEN;
         PrintData(GeneralHeatData+TOTAL_FLOW_POS, sizeof(ERROR_FLAG));
         if( 0 == memcmp(ERROR_FLAG, GeneralHeatData+TOTAL_FLOW_POS, sizeof(ERROR_FLAG)) )
         {
            memset(GeneralHeatData+TOTAL_FLOW_POS+1, 0, CAPACITY_LEN-1);
         }
         GeneralHeatData[TOTAL_FLOW_POS] = 0x01;//unit
         //adjust TotalHeat
         const uint8 TOTAL_HEAT_POS = TOTAL_FLOW_POS+CAPACITY_LEN;
         PrintData(GeneralHeatData+TOTAL_HEAT_POS, sizeof(ERROR_FLAG));
         if( 0 == memcmp(ERROR_FLAG, GeneralHeatData+TOTAL_HEAT_POS, sizeof(ERROR_FLAG)) )
         {
            memset(GeneralHeatData+TOTAL_HEAT_POS+1, 0, CAPACITY_LEN-1);
         }
         GeneralHeatData[TOTAL_HEAT_POS] = 0x01;//unit

         CPortal::GetInstance()->InsertGeneralHeatData( GeneralHeatData, sizeof(GeneralHeatData) );
      }
   }
   m_HeatLock.UnLock();
}

bool CHeatMonitor::GetStatus(StatusE& Status)
{
   printf("CHeatMonitor::GetStatus()\n");
   bool Ret = false;
   if( m_HeatLock.TryLock() )
   {
      if( false == IsStarted() )
      {
         printf("CHeatMonitor::GetStatus()----Status error\n");
         Status = STATUS_ERROR;
         Ret = true;
      }else
      {
         if( m_IsHeatInfoReady )
         {
            Status = STATUS_OFFLINE;
            for(uint32 i = 0; i < m_HeatNodeVector.size(); i++)
            {
               if(false == m_HeatNodeVector[i].IsOffline)
               {
                  Status = STATUS_OK;
                  break;
               }
            }

            Ret = true;
         }else
         {
            m_GetHeatInforTaskActive = true;
         }
      }

      m_HeatLock.UnLock();
   }

   return Ret;
}

bool CHeatMonitor::GetHeatNodeInfoList(HeatNodeInfoListT& HeatNodeInfoList)
{
   printf("CHeatMonitor::GetHeatNodeInfoList()---1\n");
   bool Ret = false;
   if(m_HeatLock.TryLock())
   {
      if( m_IsHeatInfoReady )
      {
         HeatNodeInfoList.clear();
         for(uint32 i = 0; i < m_HeatNodeVector.size(); i++)
         {
            printf("CHeatMonitor::GetHeatNodeInfoList()---2---i=%d\n", i);
            HeatNodeInfoT HeatNodeInfo;

            HeatNodeInfo.IsOffline = m_HeatNodeVector[i].IsOffline;
            //MacAddress
            memcpy(HeatNodeInfo.MacAddress, m_HeatNodeVector[i].MacAddress, sizeof(m_HeatNodeVector[i].MacAddress));

            //SupplyWaterTemperature
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].SupplyWaterTemperature, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---SupplyWaterTemperature\n");
               PrintData(m_HeatNodeVector[i].SupplyWaterTemperature, sizeof(m_HeatNodeVector[i].SupplyWaterTemperature));
               memset( HeatNodeInfo.SupplyWaterTemperature, 0, sizeof(HeatNodeInfo.SupplyWaterTemperature) );
               uint32 SupplyWaterTemperature= 10*((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-1]>>4)&0x0F)
                                              + ((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-1]&0x0F));
               SupplyWaterTemperature = 100*SupplyWaterTemperature
                                        + 10*((m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-2]>>4)&0x0F)
                                        + (m_HeatNodeVector[i].SupplyWaterTemperature[sizeof(m_HeatNodeVector[i].SupplyWaterTemperature)-2]&0x0F);
               printf("CHeatMonitor::GetHeatNodeInfoList()---SupplyWaterTemperature=%u\n", SupplyWaterTemperature);
               sprintf( HeatNodeInfo.SupplyWaterTemperature, "%d.%d", SupplyWaterTemperature/10, SupplyWaterTemperature%10 );
            }else
            {
               sprintf( HeatNodeInfo.SupplyWaterTemperature, "Err" );
            }
            //ReturnWaterTemperature
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].ReturnWaterTemperature, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---ReturnWaterTemperature\n");
               PrintData(m_HeatNodeVector[i].ReturnWaterTemperature, sizeof(m_HeatNodeVector[i].ReturnWaterTemperature));
               memset( HeatNodeInfo.ReturnWaterTemperature, 0, sizeof(HeatNodeInfo.ReturnWaterTemperature) );
               uint32 ReturnWaterTemperature= 10*((m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-1]>>4)&0x0F)
                                              + (m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-1]&0x0F);
               ReturnWaterTemperature = 100*ReturnWaterTemperature
                                        + 10*((m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-2]>>4)&0x0F)
                                        + (m_HeatNodeVector[i].ReturnWaterTemperature[sizeof(m_HeatNodeVector[i].ReturnWaterTemperature)-2]&0x0F);
               sprintf( HeatNodeInfo.ReturnWaterTemperature, "%d.%d", ReturnWaterTemperature/10, ReturnWaterTemperature%10 );
            }else
            {
               sprintf( HeatNodeInfo.ReturnWaterTemperature , "Err" );
            }
            //CurrentFlowVelocity
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].CurrentFlowVelocity, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---CurrentFlowVelocity\n");
               PrintData(m_HeatNodeVector[i].CurrentFlowVelocity, sizeof(m_HeatNodeVector[i].CurrentFlowVelocity));
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
            }else
            {
               sprintf( HeatNodeInfo.CurrentFlowVelocity, "Err" );
            }
            //CurrentHeatVelocity
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].CurrentHeatVelocity, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---CurrentHeatVelocity\n");
               PrintData(m_HeatNodeVector[i].CurrentHeatVelocity, sizeof(m_HeatNodeVector[i].CurrentHeatVelocity));
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
            }else
            {
               sprintf( HeatNodeInfo.CurrentHeatVelocity, "Err" );
            }
            //TotalFlow
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].TotalFlow, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---TotalFlow\n");
               PrintData(m_HeatNodeVector[i].TotalFlow, sizeof(m_HeatNodeVector[i].TotalFlow));
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
            }else
            {
               sprintf( HeatNodeInfo.TotalFlow, "Err" );
            }
            //TotalHeat
            if( memcmp( ERROR_FLAG, m_HeatNodeVector[i].TotalHeat, sizeof(ERROR_FLAG) ) )
            {
               printf("CHeatMonitor::GetHeatNodeInfoList()---TotalHeat\n");
               PrintData(m_HeatNodeVector[i].TotalHeat, sizeof(m_HeatNodeVector[i].TotalHeat));
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
            }else
            {
               sprintf( HeatNodeInfo.TotalHeat, "Err" );
            }

            HeatNodeInfoList.push_back(HeatNodeInfo);
         }

         Ret = true;
      }else
      {
         m_GetHeatInforTaskActive = true;
      }
      m_HeatLock.UnLock();
   }

   return Ret;
}

void CHeatMonitor::GetHeatInfoTask()
{
   printf("CHeatMonitor::GetHeatInfoTask()----m_GetHeatInforTaskActive=%d, m_IsHeatInfoReady=%d\n", m_GetHeatInforTaskActive, m_IsHeatInfoReady);
   if( m_GetHeatInforTaskActive )
   {
      if( false == m_IsHeatInfoReady )
      {
         printf("CHeatMonitor::GetHeatInfoTask()----GetHeatInfo\n");
         GetHeatData();
      }

      if( m_IsHeatInfoReady )
      {
         m_GetHeatInforTaskActive = false;
      }
   }

   if( m_HeatInfoTimeOut.Done() )
   {
      printf("CHeatMonitor::GetHeatInfoTask()----HeatInfo NOT ready\n");
      m_IsHeatInfoReady = false;
   }
}
