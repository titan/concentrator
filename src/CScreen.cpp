#include<assert.h>
#include"Utils.h"
#ifndef CFG_DATETIM_LIB
#define CFG_DATETIM_LIB
#endif
#include"libs_emsys_odm.h"
#include"CScreen.h"
#include"lcddrv.h"
#include"CHeatMonitor.h"
#include"CForwarderMonitor.h"
#include"CPortal.h"
#include "IValveMonitorFactory.h"

#ifdef DEBUG_SCREEN
#define DEBUG printf
#else
#define DEBUG(...)
#endif

const uint8 SCREEN_CHAR_WIDTH = 16;//16 char len
const uint8 SCREEN_CHAR_HEIGH = 8;//8
const uint32 MAX_HEAT_INFO_COUNT = 4;
const uint32 LCD_TIMEOUT = 60;
/****************************util function******************************/
static void PrintHZ(char x, char y, const char* pHZ, uint8 HZLen)
{
   assert(pHZ && HZLen>0);
   for(uint8 i = 0; i < HZLen; i++)
   {
      lcdwritehz(pHZ[i], y, x+i, 0);
   }
}

static void PrintChar(char x, char y, const char* pChar, uint8 CharLen)
{
   assert(pChar && CharLen>0);
   for(char i = 0; i < CharLen; i++)
   {
      lcdwritechar(pChar[i], y, x+i, 0, 0, 0, 0, 0);
   }
}

static void PrintInfo(uint8* pData, uint32 DataLen)
{
   DEBUG("PrintInfo()\n");
   if( (NULL == pData) || (0 == DataLen) )
   {
      return;
   }

   const std::string InfoComPort = "/dev/ttyS7";
   const char* InfoComConfig = "2400,8,1,N";
   static CSerialComm InfoCom(InfoComPort);
   if(false == InfoCom.IsOpen())
   {
      InfoCom.Open();
      InfoCom.SetParity( InfoComConfig );
   }

   PrintData(pData, DataLen);
   if( InfoCom.IsOpen() )
   {
      uint32 SentDataLen = DataLen;
      InfoCom.WriteBuf(pData, SentDataLen);
      DEBUG("PrintInfo()----DataLen=%d, SentDataLen=%d\n", DataLen, SentDataLen);
   }else
   {
      DEBUG("PrintInfo()----%s NOT open\n", InfoComPort.c_str());
   }
}
/***************************************************************************/
//the screen shows only one line
const char Line_1_1_X = 0;
const char Line_1_1_Y = 3;
//the screen shows two lines
const char Line_2_1_X = 0;
const char Line_2_1_Y = 1;
const char Line_2_2_X = 0;
const char Line_2_2_Y = 4;
//the screen shows three lines
const char Line_3_1_X = 0;
const char Line_3_1_Y = 1;
const char Line_3_2_X = 0;
const char Line_3_2_Y = 3;
const char Line_3_3_X = 0;
const char Line_3_3_Y = 5;

const char LineBlankDataChar[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '};
const char ConcentratorNameHZ[] = {0, 1, 2, 3, 4};/*数据集中器*/
const char ConcentratorTypeChar[] = {'H', 'D', 'R', '3', '0', '-', 'S', '0', '1'};
const char ForwarderNameHZ[] = {5, 6, 4};/*转发器*/
const char OKChar[] = {'O', 'K'};
const char BusyChar[] = {'.', '.', '.'};
const char OfflineChar[] = {'O', 'f', 'f', 'l', 'i', 'n', 'e'};
const char CheckingHZ[] = {10, 11, 3};/*检测中*/
const char ErrorChar[] = {'E', 'r', 'r'};
const char GeneralHeatNameHZ[] = {7, 8, 9};/*热总表*/
const char GPRSChar[] = {'G', 'P', 'R', 'S'};
const char GPRSServiceHZ[] = {12, 13, 14, 15};/*网络服务*/
const char NULLStrHZ[] = {16};/*无*/
const char SignalIntesityHZ[] = {17, 18, 19, 48};/*信号强度*/
const char SignalIntesityBusy[] = {'B', 'u', 's', 'y'};/*信号强度*/
const char RemoteConnectHZ[] = {20, 21, 22, 23};/*远程连接*/
const char RemoteConnectingHZ[] = {20, 21, 22, 23, 3};/*远程连接中*/
const char ServerConnectionSuccessHZ[] = {14, 15, 9, 22, 23, 24, 25};/*服务器连接成功*/
const char ServerConnectionFailureHZ[] = {14, 15, 9, 22, 23, 26, 27};/*服务器连接失败*/
const char ServerConnectionStatusHZ[] = {14, 15, 9, 22, 23, 28, 29};/*服务器连接状态*/
const char ConnectedHZ[] = {30, 22, 23};/*已连接*/
const char NotConnectedHZ[] = {31, 22, 23};/*未连接*/
const char NormalHZ[] = {76, 113};/*正常*/
const char ValveCountHZ[] = {32, 33, 4, 0, 34};/*阀控器数量*/
const char WarnningInfoHZ[] = {35, 36, 17, 37};/*报警信息*/
const char ServerOffLineHZ[] = {14, 15, 9, 38, 39};/*服务器离线*/
const char GeneralHeatErrHZ[] = {7, 8, 9, 40, 41};/*热总表故障*/
const char GeneralHeatDataHZ[] = {7, 8, 9};/*热总表*/
const char SupplyWaterTemperatureHZ[] = {42, 44, 47, 48};/*供水温度*/
const char ReturnWaterTemperatureHZ[] = {43, 44, 47, 48};/*回水温度*/
const char TemperatureUnitHZ[] = {60};/*摄氏度*/
const char TransientHeatHZ[] = {57, 51, 7, 34};/*瞬时热量*/
const char HeatUnitChar[] = {'G', 'J'};
const char TransientFlowHZ[] = {57, 51, 56, 34};/*瞬时流量*/
const char FlowUnitHZ[] = {112};/*瞬时流量*/
const char TotalHeatHZ[] = {45, 46, 7, 34};/*累计热量*/
const char TotalFlowHZ[] = {45, 46, 56, 34};/*累计流量*/

CScreen* CScreen::m_Instance = NULL;
CLock CScreen::m_ScreenLock;
CScreen* CScreen::GetInstance()
{
   if( NULL == m_Instance )
   {
      m_ScreenLock.Lock();
      if( NULL == m_Instance )
      {
         m_Instance = new CScreen();
      }
      m_ScreenLock.UnLock();
   }
   return m_Instance;
}

void CScreen::PowerOn()
{
   DEBUG("CScreen::PowerOn()\n");
   //init LCD
   LcdGpin();
   Lcd_TestBuf();
   LcdInit(1);
   LCD_clear();

   m_Timer.StartTimer( LCD_TIMEOUT );
}

void CScreen::InputKey(KeyE KeyValue)
{
   DEBUG("CScreen::InputKey(%d)\n", KeyValue);
   m_ScreenLock.Lock();
   m_Timer.StartTimer( LCD_TIMEOUT );
   LcdLedOn();
   switch( KeyValue )
   {
      case KEY_LEFT:
         {
            if(m_SubScreen>0)
            {
               m_SubScreen--;

               ClearScreen();
               ClearLineDraw();
            }
         }
         break;

      case KEY_RIGHT:
         {
            m_SubScreen++;

            ClearScreen();
            ClearLineDraw();
         }
         break;

      case KEY_DOWN:
         {
            m_SubScreen = 0;
            //up to the heat count
            m_CurrentScreen++;

            ClearScreen();
            ClearLineData();
         }
         break;

      case KEY_UP:
         {
            m_SubScreen = 0;
            if(m_CurrentScreen > 0)
            {
               m_CurrentScreen--;

               ClearScreen();
               ClearLineData();
            }
         }
         break;
   default:
       break;
   }

   m_ScreenLock.UnLock();
}

void CScreen::Draw()
{
   DEBUG("CScreen::Draw()----m_CurrentScreen=%d, m_SubScreen=%d\n", m_CurrentScreen, m_SubScreen);
   m_ScreenLock.Lock();
   if( m_Timer.Done() )
   {
      LcdLedOff();
      m_CurrentScreen = 0;
      ClearScreen();
      ClearLineData();
   }
   switch(m_CurrentScreen)
   {
      case SCREEN_POWERON:
         {
            DrawPowerOnInit();
         }
         break;

      case SCREEN_MACHINE_STATUS:
         {
            DrawMachineStatus();
         }
         break;

      case SCREEN_GPRS_STATUS:
         {
            DrawGPRSStatus();
         }
         break;

      case SCREEN_FORWARDER_INFO:
         {
            DrawForwarderINFO();
         }
         break;

      default:
         {
            DrawGeneralHeatINFO();
         }
         break;
   }
   m_ScreenLock.UnLock();
}

/********************************Draw related functions*******************************************/
bool CScreen::IsDataInvalid()
{
   if(m_IsFirstLineDataReady && m_IsSecondLineDataReady && m_IsThirdLineDataReady)
   {
      return false;
   }else
   {
      return true;
   }
}

void CScreen::ClearScreen()
{
   LCD_clear();
}

void CScreen::ClearScreenLineRect(ScreenLineE nLineIndex)
{
   switch( nLineIndex )
   {
      case SCREEN_LINE_1_1:
         PrintChar(0, Line_1_1_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      case SCREEN_LINE_2_1:
         PrintChar(0, Line_2_1_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      case SCREEN_LINE_2_2:
         PrintChar(0, Line_2_2_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      case SCREEN_LINE_3_1:
         PrintChar(0, Line_3_1_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      case SCREEN_LINE_3_2:
         PrintChar(0, Line_3_2_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      case SCREEN_LINE_3_3:
         PrintChar(0, Line_3_3_Y, LineBlankDataChar, sizeof(LineBlankDataChar));
         break;

      default:
         break;
   }
}

void CScreen::DrawPowerOnInit()
{
   DEBUG("CScreen::DrawPowerOnInit()\n");
   if(false == m_IsFirstLineDrawn)
   {
      ClearScreenLineRect(SCREEN_LINE_3_1);
      PrintHZ(Line_3_1_X, Line_3_1_Y, ConcentratorNameHZ, sizeof(ConcentratorNameHZ));
      char VersionStr[16] = {0};
      sprintf(VersionStr, "v%02d.%02d", VERSION, SUBVERSION);
      PrintChar(Line_3_1_X+2*sizeof(ConcentratorNameHZ), Line_3_1_Y, VersionStr, strlen(VersionStr));
      m_IsFirstLineDrawn = true;
   }

   if(false == m_IsSecondLineDrawn)
   {
      ClearScreenLineRect(SCREEN_LINE_3_2);
      PrintChar(2*Line_3_2_X, Line_3_2_Y, ConcentratorTypeChar, sizeof(ConcentratorTypeChar));
      m_IsSecondLineDrawn = true;
   }

   //always update
   ClearScreenLineRect(SCREEN_LINE_3_3);
   uint32 TimeData[7] = {0};
   if(ERROR_OK != GetDateTime(TimeData, 1))//RTC
   {
      DEBUG("DrawPowerOnInit()----Can't read time from RTC\n");
      return;
   }
   char DateTimeStr[32] = {0};
   sprintf(DateTimeStr, "%04d-%02d-%02d %02d:%02d", TimeData[0], TimeData[1], TimeData[2], TimeData[3], TimeData[4]);
   PrintChar(2*Line_3_3_X, Line_3_3_Y, DateTimeStr, strlen(DateTimeStr));
}

void CScreen::DrawMachineStatus()
{
   DEBUG("CScreen::DrawMachineStatus()\n");
   char MaxLen = sizeof(ForwarderNameHZ)>sizeof(GeneralHeatNameHZ)?sizeof(ForwarderNameHZ):sizeof(GeneralHeatNameHZ);
   MaxLen = MaxLen>sizeof(GPRSChar)?MaxLen:sizeof(GPRSChar);

   //Line 1:Forwarder status
   static Status ForwarderStatus = STATUS_CHECKING;
   if(false == m_IsFirstLineDataReady)
   {
      ForwarderStatus = IValveMonitorFactory::GetInstance()->GetStatus();
      m_IsFirstLineDataReady = true;
      if(m_IsFirstLineDataReady)
      {
         m_IsFirstLineDrawn = false;
      }
   }
   if( m_IsFirstLineDataReady )
   {
      if(false == m_IsFirstLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_1);
         PrintHZ(Line_3_1_X, Line_3_1_Y, ForwarderNameHZ, sizeof(ForwarderNameHZ));
         switch( ForwarderStatus )
         {
            case STATUS_OK:
               {
                  PrintChar(2*Line_3_1_X+1/*space*/+2*MaxLen, Line_3_1_Y, OKChar, sizeof(OKChar));
               }
               break;

            case STATUS_CHECKING:
               {
                  PrintHZ(Line_3_1_X+1/*space*/+MaxLen, Line_3_1_Y, CheckingHZ, sizeof(CheckingHZ));
               }
               break;

            case STATUS_OFFLINE:
               {
                  PrintChar(2*Line_3_1_X+1/*space*/+2*MaxLen, Line_3_1_Y, OfflineChar, sizeof(OfflineChar));
               }
               break;

            case STATUS_ERROR:
            default:
               {
                  PrintChar(2*Line_3_1_X+1/*space*/+2*MaxLen, Line_3_1_Y, ErrorChar, sizeof(ErrorChar));
               }
               break;
         }
         m_IsFirstLineDrawn = true;
      }
   }else
   {
      if(false == m_IsFirstLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_1);
         PrintHZ(Line_3_1_X, Line_3_1_Y, ForwarderNameHZ, sizeof(ForwarderNameHZ));
         PrintHZ(Line_3_1_X+1/*space*/+MaxLen, Line_3_1_Y, CheckingHZ, sizeof(CheckingHZ));
         m_IsFirstLineDrawn = true;
      }
   }

   //Line 2:General heat status
   static Status GeneralHeatStatus = STATUS_CHECKING;
   if(false == m_IsSecondLineDataReady)
   {
      m_IsSecondLineDataReady = CHeatMonitor::GetInstance()->GetStatus(GeneralHeatStatus);
      if(m_IsSecondLineDataReady)
      {
         m_IsSecondLineDrawn = false;
      }
   }
   if(m_IsSecondLineDataReady)
   {
      if(false == m_IsSecondLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_2);
         PrintHZ(Line_3_2_X, Line_3_2_Y, GeneralHeatNameHZ, sizeof(GeneralHeatNameHZ));
         switch( GeneralHeatStatus )
         {
            case STATUS_OK:
               {
                  PrintChar(2*Line_3_2_X+1/*space*/+2*MaxLen, Line_3_2_Y, OKChar, sizeof(OKChar));
               }
               break;

            case STATUS_CHECKING:
               {
                  PrintHZ(Line_3_2_X+1/*space*/+MaxLen, Line_3_2_Y, CheckingHZ, sizeof(CheckingHZ));
               }
               break;

            case STATUS_OFFLINE:
               {
                  PrintChar(2*Line_3_2_X+1/*space*/+2*MaxLen, Line_3_2_Y, OfflineChar, sizeof(OfflineChar));
               }
               break;

            case STATUS_ERROR:
            default:
               {
                  PrintChar(2*Line_3_2_X+1/*space*/+2*MaxLen, Line_3_2_Y, ErrorChar, sizeof(ErrorChar));
               }
               break;
         }
         m_IsSecondLineDrawn = true;
      }
   }else
   {
      if(false == m_IsSecondLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_2);
         PrintHZ(Line_3_2_X, Line_3_2_Y, GeneralHeatNameHZ, sizeof(GeneralHeatNameHZ));
         PrintHZ(Line_3_2_X+1/*space*/+MaxLen, Line_3_2_Y, CheckingHZ, sizeof(CheckingHZ));
         m_IsSecondLineDrawn = true;
      }
   }

   //Line 3:GPRS
   DEBUG("CScreen::DrawMachineStatus()----print GPRS\n");
   static Status GPRSStatus = STATUS_CHECKING;
   if(false == m_IsThirdLineDataReady)
   {
      m_IsThirdLineDataReady = CPortal::GetInstance()->GetGPRSStatus(GPRSStatus);
      if(m_IsThirdLineDataReady)
      {
         m_IsThirdLineDrawn = false;
      }
   }
   if(m_IsThirdLineDataReady)
   {
      if(false == m_IsThirdLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_3);
         PrintChar(Line_3_3_X, Line_3_3_Y, GPRSChar, sizeof(GPRSChar));
         switch( GPRSStatus )
         {
            case STATUS_OK:
               {
                  DEBUG("CScreen::DrawMachineStatus()----GPRS OK\n");
                  PrintChar(2*Line_3_3_X+1/*space*/+2*MaxLen, Line_3_3_Y, OKChar, sizeof(OKChar));
               }
               break;

            case STATUS_CHECKING:
               {
                  DEBUG("CScreen::DrawMachineStatus()----GPRS checking\n");
                  PrintHZ(Line_3_3_X+1/*space*/+MaxLen, Line_3_3_Y, CheckingHZ, sizeof(CheckingHZ));
               }
               break;

            case STATUS_OFFLINE:
               {
                  PrintChar(2*Line_3_3_X+1/*space*/+2*MaxLen, Line_3_3_Y, OfflineChar, sizeof(OfflineChar));
               }
               break;

            case STATUS_ERROR:
            default:
               {
                  DEBUG("CScreen::DrawMachineStatus()----GPRS error\n");
                  PrintChar(2*Line_3_3_X+1/*space*/+2*MaxLen, Line_3_3_Y, ErrorChar, sizeof(ErrorChar));
               }
               break;
         }
         m_IsThirdLineDrawn = true;
      }
   }else
   {
      DEBUG("CScreen::DrawMachineStatus()----GPRS checking\n");
      if(false == m_IsThirdLineDrawn)
      {
         ClearScreenLineRect(SCREEN_LINE_3_3);
         PrintChar(Line_3_3_X, Line_3_3_Y, GPRSChar, sizeof(GPRSChar));
         PrintHZ(Line_3_3_X+1/*space*/+MaxLen, Line_3_3_Y, CheckingHZ, sizeof(CheckingHZ));
         m_IsThirdLineDrawn = true;
      }
   }

   DEBUG("CScreen::DrawMachineStatus()----End\n");
}

void CScreen::DrawGPRSStatus()
{
   DEBUG("CScreen::DrawGPRSStatus()\n");
   static Status GPRSStatus = STATUS_ERROR;
   if(false == m_IsFirstLineDataReady)
   {
      m_IsFirstLineDataReady = CPortal::GetInstance()->GetGPRSStatus(GPRSStatus);
      if(m_IsFirstLineDataReady)
      {
         ClearScreen();
         m_IsFirstLineDrawn = false;
      }
   }

   if(m_IsFirstLineDataReady)
   {
      if(false == m_IsFirstLineDrawn)
      {
         switch( GPRSStatus )
         {
            case STATUS_OK:
               {
                  if(false == m_IsThirdLineDrawn)
                  {
                     //Line 1
                     PrintHZ(Line_3_1_X, Line_3_1_Y, GPRSServiceHZ, sizeof(GPRSServiceHZ));
                     PrintChar(2*(Line_3_1_X+sizeof(GPRSServiceHZ)), Line_3_1_Y, ":", 1);
                     PrintHZ(Line_3_1_X+sizeof(GPRSServiceHZ)+1, Line_3_1_Y, NormalHZ, sizeof(NormalHZ));
                     //Line 3
                     PrintHZ(Line_3_3_X, Line_3_3_Y, RemoteConnectingHZ, sizeof(RemoteConnectingHZ));
                     char Ellipsis[] = "......";
                     PrintChar( 2*(Line_3_3_X+sizeof(RemoteConnectingHZ)), Line_3_3_Y, Ellipsis, strlen(Ellipsis));

                     m_IsThirdLineDrawn = true;
                  }
                  //Line 2
                  static uint8 nSignalIntesity = UNKNOWN_SIGNAL_INTESITY;
                  if(false == m_IsSecondLineDataReady)
                  {
                     m_IsSecondLineDataReady = CPortal::GetInstance()->GetGPRSSignalIntesity(nSignalIntesity);
                     if(m_IsSecondLineDataReady)
                     {
                        m_IsSecondLineDrawn = false;
                     }
                  }
                  if(m_IsSecondLineDataReady)
                  {
                     if(false == m_IsSecondLineDrawn)
                     {
                        PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
                        PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
                        char SignalIntesityStr[4] = {0};
                        if(UNKNOWN_SIGNAL_INTESITY == nSignalIntesity)
                        {
                           PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ))+1, Line_3_2_Y, SignalIntesityBusy, strlen(SignalIntesityBusy));
                        }else
                        {
                           sprintf( SignalIntesityStr, "%02d", nSignalIntesity );
                           PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ))+1, Line_3_2_Y, SignalIntesityStr, strlen(SignalIntesityStr));
                        }
                        m_IsSecondLineDrawn = true;
                     }
                  }else
                  {
                     if(false == m_IsSecondLineDrawn)
                     {
                        PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
                        PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
                        PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
                        PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
                        PrintHZ(Line_3_2_X+sizeof(SignalIntesityHZ)+1, Line_3_2_Y, CheckingHZ, sizeof(CheckingHZ));

                        m_IsSecondLineDrawn = true;
                     }
                  }

                  m_IsFirstLineDrawn = true;
               }
               break;

            case STATUS_CHECKING:
               {
                  //Line 1
                  PrintHZ(Line_3_1_X, Line_3_1_Y, GPRSServiceHZ, sizeof(GPRSServiceHZ));
                  PrintChar(2*(Line_3_1_X+sizeof(GPRSServiceHZ)), Line_3_1_Y, ":", 1);
                  PrintHZ(Line_3_1_X+sizeof(GPRSServiceHZ)+1, Line_3_1_Y, CheckingHZ, sizeof(CheckingHZ));
                  //Line 2
                  PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
                  PrintChar( 2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
                  PrintChar(2*(Line_3_2_X+sizeof(SignalIntesityHZ))+1, Line_3_2_Y, BusyChar, sizeof(BusyChar));
                  //Line 3
                  PrintHZ(Line_3_3_X, Line_3_3_Y, RemoteConnectingHZ, sizeof(RemoteConnectingHZ));
                  char Ellipsis[] = "......";
                  PrintChar( 2*(Line_3_3_X+sizeof(RemoteConnectingHZ)), Line_3_3_Y, Ellipsis, strlen(Ellipsis));

                  m_IsFirstLineDrawn = true;
               }
               break;

            case STATUS_ERROR:
            default:
               {
                  //Line 1
                  PrintHZ(Line_3_1_X, Line_3_1_Y, GPRSServiceHZ, sizeof(GPRSServiceHZ));
                  PrintChar(2*(Line_3_1_X+sizeof(GPRSServiceHZ)), Line_3_1_Y, ":", 1);
                  PrintHZ(Line_3_1_X+sizeof(GPRSServiceHZ)+1, Line_3_1_Y, NULLStrHZ, sizeof(NULLStrHZ));
                  //Line 2
                  PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
                  PrintChar(2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
                  //Line 3
                  PrintHZ(Line_3_3_X, Line_3_3_Y, RemoteConnectHZ, sizeof(RemoteConnectHZ));
                  PrintChar(2*(Line_3_3_X+sizeof(RemoteConnectHZ)), Line_3_3_Y, ":", 1);

                  m_IsFirstLineDrawn = true;
               }
               break;
         }
      }
   }else
   {
      if(false == m_IsFirstLineDrawn)
      {
         //Line 1
         PrintHZ(Line_3_1_X, Line_3_1_Y, GPRSServiceHZ, sizeof(GPRSServiceHZ));
         PrintChar(2*(Line_3_1_X+sizeof(GPRSServiceHZ)), Line_3_1_Y, ":", 1);
         PrintHZ(Line_3_1_X+sizeof(GPRSServiceHZ)+1, Line_3_1_Y, CheckingHZ, sizeof(CheckingHZ));
         //Line 2
         PrintHZ(Line_3_2_X, Line_3_2_Y, SignalIntesityHZ, sizeof(SignalIntesityHZ));
         PrintChar(2*(Line_3_2_X+sizeof(SignalIntesityHZ)), Line_3_2_Y, ":", 1);
         PrintChar(2*(Line_3_2_X+sizeof(SignalIntesityHZ))+1, Line_3_2_Y, BusyChar, sizeof(BusyChar));
         //Line 3
         PrintHZ(Line_3_3_X, Line_3_3_Y, RemoteConnectHZ, sizeof(RemoteConnectHZ));
         PrintChar(2*(Line_3_3_X+sizeof(RemoteConnectHZ)), Line_3_3_Y, ":", 1);
         PrintChar(2*(Line_3_3_X+sizeof(RemoteConnectHZ))+1, Line_3_3_Y, BusyChar, sizeof(BusyChar));

         m_IsFirstLineDrawn = true;
      }
   }
}

void CScreen::DrawGeneralHeatINFO()
{
   DEBUG("CScreen::DrawGeneralHeatINFO()\n");
   static HeatNodeInfoListT HeatNodeInfoList;
   //static Status HeatStatus = STATUS_ERROR;
   if(false == m_IsFirstLineDataReady)
   {
      m_IsFirstLineDataReady = CHeatMonitor::GetInstance()->GetHeatNodeInfoList(HeatNodeInfoList);
      if(m_IsFirstLineDataReady)
      {
         ClearScreen();
         m_IsFirstLineDrawn = false;
      }
   }

   if(m_IsFirstLineDataReady)
   {
      if(false == m_IsFirstLineDrawn)
      {

         if(0 == HeatNodeInfoList.size())
         {//only attach one heat monitor
            //line 1
            PrintHZ(Line_3_1_X, Line_3_1_Y, GeneralHeatDataHZ, sizeof(GeneralHeatDataHZ));
            PrintChar(2*Line_3_1_X+2*sizeof(GeneralHeatDataHZ), Line_3_1_Y, ":", 1);
            //line 2
            PrintChar(Line_3_2_X, Line_3_2_Y, ErrorChar, sizeof(ErrorChar));
         }else
         {
            uint32 CurrentHeatNodeIndex = m_CurrentScreen - SCREEN_GENERALHEAT_INFO;
            if( CurrentHeatNodeIndex >= HeatNodeInfoList.size() )
            {
               CurrentHeatNodeIndex = HeatNodeInfoList.size()-1;
            }

            //line 1
            PrintHZ(Line_3_1_X, Line_3_1_Y, GeneralHeatDataHZ, sizeof(GeneralHeatDataHZ));
            char TempStr[128] = {0};
            sprintf( TempStr, "%d-%d(0x%02X):", HeatNodeInfoList.size(), CurrentHeatNodeIndex+1, HeatNodeInfoList[CurrentHeatNodeIndex].MacAddress[0] );
            PrintChar(2*Line_3_1_X+2*sizeof(GeneralHeatDataHZ), Line_3_1_Y, TempStr, strlen(TempStr));

            if(HeatNodeInfoList[CurrentHeatNodeIndex].IsOffline)
            {
               PrintChar(Line_3_2_X, Line_3_2_Y, OfflineChar, sizeof(OfflineChar));
            }else
            {
               if(m_SubScreen >= MAX_HEAT_INFO_COUNT)
               {
                  m_SubScreen = MAX_HEAT_INFO_COUNT-1;
               }

               switch(m_SubScreen)
               {
                  default:
                  case 0:
                     {//SupplyWaterTemperatureHZ && ReturnWaterTemperatureHZ
                        //Line 2
                        PrintHZ(Line_3_2_X, Line_3_2_Y, SupplyWaterTemperatureHZ, sizeof(SupplyWaterTemperatureHZ));
                        PrintChar(2*Line_3_2_X+2*sizeof(SupplyWaterTemperatureHZ), Line_3_2_Y, ":", 1);
                        PrintChar(2*Line_3_2_X+2*sizeof(SupplyWaterTemperatureHZ)+1, Line_3_2_Y, HeatNodeInfoList[CurrentHeatNodeIndex].SupplyWaterTemperature, strlen(HeatNodeInfoList[CurrentHeatNodeIndex].SupplyWaterTemperature));
                        PrintHZ(Line_3_2_X+sizeof(SupplyWaterTemperatureHZ)+1+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].SupplyWaterTemperature)/2
                                , Line_3_2_Y, TemperatureUnitHZ, sizeof(TemperatureUnitHZ));

                        //Line 3
                        PrintHZ(Line_3_3_X, Line_3_3_Y, ReturnWaterTemperatureHZ, sizeof(ReturnWaterTemperatureHZ));
                        PrintChar(2*Line_3_3_X+2*sizeof(ReturnWaterTemperatureHZ), Line_3_3_Y, ":", 1);
                        PrintChar(2*Line_3_3_X+2*sizeof(ReturnWaterTemperatureHZ)+1, Line_3_3_Y, HeatNodeInfoList[CurrentHeatNodeIndex].ReturnWaterTemperature, strlen(HeatNodeInfoList[CurrentHeatNodeIndex].ReturnWaterTemperature));
                        PrintHZ(Line_3_3_X+sizeof(ReturnWaterTemperatureHZ)+1+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].ReturnWaterTemperature)/2, Line_3_3_Y, TemperatureUnitHZ, sizeof(TemperatureUnitHZ));
                     }
                     break;

                  case 1:
                     {//Current Heat
                        PrintHZ(Line_3_2_X, Line_3_2_Y, TransientHeatHZ, sizeof(TransientHeatHZ));
                        PrintChar(2*Line_3_2_X+2*sizeof(TransientHeatHZ), Line_3_2_Y, ":", 1);
                        PrintChar(2*Line_3_3_X+2, Line_3_3_Y, HeatNodeInfoList[CurrentHeatNodeIndex].CurrentHeatVelocity, strlen(HeatNodeInfoList[CurrentHeatNodeIndex].CurrentHeatVelocity));
                        PrintChar(2*Line_3_3_X+2+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].CurrentHeatVelocity)+1, Line_3_3_Y, HeatUnitChar, sizeof(HeatUnitChar));
                     }
                     break;

                  case 2:
                     {//Current Flow
                        PrintHZ(Line_3_2_X, Line_3_2_Y, TransientFlowHZ, sizeof(TransientFlowHZ));
                        PrintChar(2*Line_3_2_X+2*sizeof(TransientFlowHZ), Line_3_2_Y, ":", 1);
                        PrintChar(2*Line_3_3_X+2, Line_3_3_Y, HeatNodeInfoList[CurrentHeatNodeIndex].CurrentFlowVelocity, strlen(HeatNodeInfoList[CurrentHeatNodeIndex].CurrentFlowVelocity));
                        PrintHZ(Line_3_3_X+2+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].CurrentFlowVelocity)/2, Line_3_3_Y, FlowUnitHZ, sizeof(FlowUnitHZ));
                     }
                     break;

                  case 3:
                     {//TotalHeatHZ
                        PrintHZ(Line_3_2_X, Line_3_2_Y, TotalHeatHZ, sizeof(TotalHeatHZ));
                        PrintChar(2*Line_3_2_X+2*sizeof(TotalHeatHZ), Line_3_2_Y, ":", 1);
                        PrintChar(2*Line_3_3_X+2, Line_3_3_Y, HeatNodeInfoList[CurrentHeatNodeIndex].TotalHeat, strlen(HeatNodeInfoList[CurrentHeatNodeIndex].TotalHeat));
                        PrintChar(2*Line_3_3_X+2+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].TotalHeat)+1, Line_3_3_Y, HeatUnitChar, sizeof(HeatUnitChar));
                     }
                     break;

                  case 4:
                     {//TotalFlowHZ
                        PrintHZ(Line_3_2_X, Line_3_2_Y, TotalFlowHZ, sizeof(TotalFlowHZ));
                        PrintChar(2*Line_3_2_X+2*sizeof(TotalFlowHZ), Line_3_2_Y, ":", 1);
                        PrintChar(2*Line_3_3_X+2, Line_3_3_Y, HeatNodeInfoList[CurrentHeatNodeIndex].TotalFlow, sizeof(HeatNodeInfoList[CurrentHeatNodeIndex].TotalFlow));
                        PrintHZ(Line_3_3_X+2+strlen(HeatNodeInfoList[CurrentHeatNodeIndex].TotalFlow), Line_3_3_Y, FlowUnitHZ, sizeof(FlowUnitHZ));
                     }
                     break;

               }
            }
         }

         m_IsFirstLineDrawn = true;
      }
   }else
   {
      if(false == m_IsFirstLineDrawn)
      {
         //line 1
         PrintHZ(Line_3_1_X, Line_3_1_Y, GeneralHeatDataHZ, sizeof(GeneralHeatDataHZ));
         PrintChar(2*Line_3_1_X+2*sizeof(GeneralHeatDataHZ), Line_3_1_Y, ":", 1);
         //line 2
         PrintHZ(Line_3_2_X, Line_3_2_Y, CheckingHZ, sizeof(CheckingHZ));

         m_IsFirstLineDrawn = true;
      }
   }
}

void CScreen::DrawForwarderINFO()
{
   DEBUG("CScreen::DrawForwarderINFO()\n");
   static ForwarderInfoListT ForwarderInfoList;
   if(false == m_IsFirstLineDataReady)
   {
      m_IsFirstLineDataReady = CForwarderMonitor::GetInstance()->GetForwarderInfo(ForwarderInfoList);
      if(m_IsFirstLineDataReady)
      {
         ClearScreen();
         m_IsFirstLineDrawn = false;
      }
   }

   if(m_IsFirstLineDataReady)
   {
      if(false == m_IsFirstLineDrawn)
      {
         char ForwarderInfoStr[64] = {0};
         if( ForwarderInfoList.size() > 0 )
         {
            if( m_SubScreen >= ForwarderInfoList.size() )
            {
               m_SubScreen = ForwarderInfoList.size()-1;
            }

            DEBUG("CScreen::DrawForwarderINFO()----1\n");
            //print out this forwarder's A1ResponseStr
            PrintInfo(ForwarderInfoList[m_SubScreen].A1ResponseStr, ForwarderInfoList[m_SubScreen].A1ResponseStrLen);

            DEBUG("CScreen::DrawForwarderINFO()----2\n");
            //Line 1
            PrintHZ(Line_2_1_X, Line_2_1_Y, ForwarderNameHZ, sizeof(ForwarderNameHZ));
            uint16 ForwarderID = (ForwarderInfoList[m_SubScreen].ForwarderID>>16) & 0xFFFF;
            ForwarderID = ((ForwarderID&0xFF)<<8) | ((ForwarderID>>8)&0xFF);
            sprintf(ForwarderInfoStr, "%d-%d:%04X", ForwarderInfoList.size(), m_SubScreen+1, ForwarderID);
            PrintChar(2*Line_2_1_X+2*sizeof(ForwarderNameHZ)+1/*space*/, Line_2_1_Y, ForwarderInfoStr, strlen(ForwarderInfoStr));

            //Line 2
            if( false == ForwarderInfoList[m_SubScreen].IsOffline )
            {
               PrintHZ(Line_2_2_X, Line_2_2_Y, ValveCountHZ, sizeof(ValveCountHZ));
               PrintChar(2*Line_2_2_X+2*sizeof(ValveCountHZ), Line_2_2_Y, ":", 1);
               char ValveCountStr[4] = {0};
               sprintf(ValveCountStr, "%d", ForwarderInfoList[m_SubScreen].ValveCount);
               PrintChar(2*Line_2_2_X+2*sizeof(ValveCountHZ)+1, Line_2_2_Y, ValveCountStr, strlen(ValveCountStr));
            }else
            {
               PrintChar(Line_2_2_X, Line_2_2_Y, OfflineChar, sizeof(OfflineChar));
            }
         }else
         {
            //Line 1
            PrintHZ(Line_2_1_X, Line_2_1_Y, ForwarderNameHZ, sizeof(ForwarderNameHZ));
            sprintf(ForwarderInfoStr, "0-0");
            PrintChar(2*Line_2_1_X+2*sizeof(ForwarderNameHZ)+1/*space*/, Line_2_1_Y, ForwarderInfoStr, strlen(ForwarderInfoStr));
         }
      }
      m_IsFirstLineDrawn = true;
   }else
   {
      if(false == m_IsFirstLineDrawn)
      {
         PrintHZ(Line_2_1_X, Line_2_1_Y, ForwarderNameHZ, sizeof(ForwarderNameHZ));
         PrintChar(2*(Line_2_1_X+sizeof(ForwarderNameHZ)), Line_2_1_Y, ":", 1);
         PrintHZ(Line_2_1_X+sizeof(ForwarderNameHZ)+1, Line_2_1_Y, CheckingHZ, sizeof(CheckingHZ));
      }
      m_IsFirstLineDrawn = true;
   }

   m_IsFirstLineDrawn = true;
}

void CScreen::ClearLineData()
{
   m_IsFirstLineDataReady = false;
   m_IsSecondLineDataReady = false;
   m_IsThirdLineDataReady = false;
   ClearLineDraw();
}

void CScreen::ClearLineDraw()
{
   m_IsFirstLineDrawn = false;
   m_IsSecondLineDrawn = false;
   m_IsThirdLineDrawn = false;
}
