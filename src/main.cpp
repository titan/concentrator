#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include<string>
#include"CHeatMonitor.h"
#include"CForwarderMonitor.h"
#include"CPortal.h"
#include"CINI.h"
#include"Utils.h"
#include "CCardHost.h"
#include "CValveMonitor.h"
#include "CMainLoop.h"
#include "sysdefs.h"

#ifdef DEBUG_MAIN
#define DEBUG(...) do {printf("%s::%s----", __FILE__, __func__);printf(__VA_ARGS__);} while(false)
#else
#define DEBUG(...)
#endif

using namespace std;
const string COM_ADDRESS_KEY = "DEVICE";
const string COM_CONFIG_KEY = "DEV_CONFIG";
const string ID_ID_FORMAT = "DEV_ID%d";
const string INVALID_KEY_VALUE = "NULL";
const string TIME_SESSION = "TIMER";
const string TIME_START_KEY = "STIM";
const string TIME_INTERVAL_KEY = "ITIM";
const uint32 DefaultStartTime = 0;
const uint32 DefaultIntervalTime = 30;
/*********************************Forwarder**********************************/
const string FORWARD_SESSION = "COLLECT";
const string FORWARD_ID_FORMAT = "%x";
const string DefaultForwardComConfig = "2400,8,1,N";
const string DefaultForwardComAddress = "/dev/ttyS5";
const uint32 MAX_FORWARDER_COUNT = 20;
const uint32 MAX_COUNT_LEN = 32;
/*********************************General Heat**********************************/
const string HEAT_SESSION = "METERS";
const string HEAT_ID_FORMAT = "%c%c%c%c%c%c%c%c";
const string DefaultGeneralHeatComAddress = "/dev/ttyS2";
const string DefaultGeneralHeatComConfig = "2400,8,1,E";
const uint32 MAX_HEAT_COUNT = 20;
/*********************************GPRS**********************************/
const string GPRS_SESSION = "GPRS";
const string GPRS_HEART_INTERVAL = "HEART_TIM";
const string GPRS_LOCAL_PORT = "LOCAL_PORT";
const string GPRS_SERVER_IP = "SERVER_IP";
const string GPRS_SERVER_PORT = "SERVER_PORT";
const string DefaultGPRSComConfig = "115200";
const string DefaultGPRSComAddress = "/dev/ttyS3";

static void TrimInvalidChar(string& Str);
static void StartForwarder();
static void StartGeneralHeat();
static void StartPortal();
static void StartCardHost();
static void StartValveMonitor();

bool wireless = true;
int heatCount = 0;
bool cardhostEnabled = false;
gpio_attr_t gpioattr;
int id = 0;

int main() {

    gpioattr.mode = PIO_MODE_OUT;
    gpioattr.resis = PIO_RESISTOR_DOWN;
    gpioattr.filter = PIO_FILTER_NOEFFECT;
    gpioattr.multer = PIO_MULDRIVER_NOEFFECT;

    string device;
    string cfg;

    SetLight(LIGHT_WARNING, false);//turn off warning light
    StartGeneralHeat(); // must be in front of StartValveMonitor
    if (access("config.ini", F_OK) == -1) {
        wireless = true;
        StartForwarder();
        device = "/dev/ttyS7";
        cfg = "2400,8,1,N";
    } else {
        CINI ini("config.ini");
        if (ini.GetValueBool("DEFAULT", "WIRELESS", true)) {
            wireless = true;
            StartForwarder();
        } else {
            wireless = false;
            StartValveMonitor();
        }

        device = ini.GetValueString("DEFAULT", "DEVICE", "/dev/ttyS7");
        TrimInvalidChar(device);
        cfg = ini.GetValueString("DEFAULT", "DEVCFG", "2400,8,1,N");
        TrimInvalidChar(cfg);
        id = ini.GetValueInt("DEFAULT", "JZQID", 0);
    }
    StartPortal();
    StartCardHost();

    int com = OpenCom((char *)device.c_str(), (char *)cfg.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
    if (com == -1) {
        DEBUG("Initilize %s with %s failed!\n", device.c_str(), cfg.c_str());
    } else {
        DEBUG("Initilize %s(%s)\n", device.c_str(), cfg.c_str());
    }

    CMainLoop::GetInstance()->SetCom(com);
    CMainLoop::GetInstance()->Run();

    return 0;
}

static void TrimInvalidChar(string& Str)
{
   uint32 Begin = 0;
   for(; Begin<Str.size(); Begin++)
   {
      if( IsChar(Str[Begin]) )
      {
         break;
      }
   }
   if(Begin >= Str.size())
   {
      return;
   }

   uint32 End = Str.size()-1;
   for(; End>Begin; End--)
   {
      if( IsChar(Str[End]) )
      {
         break;
      }
   }
   if(End<Begin)
   {
      return;
   }

   DEBUG("Str Begin=%d, End=%d\n", Begin, End);
   Str = Str.substr(Begin, End+1);
}

void StartForwarder()
{
   //config Forwarder
   CINI ForwardINI("collect_cfg.ini");
   string SectionStr = FORWARD_SESSION;
   string Key = COM_ADDRESS_KEY;
   string KeyValue = ForwardINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);

   SetPIOCfg(getGPIO(KeyValue.c_str()), gpioattr);
   CForwarderMonitor::GetInstance()->SetGPIO(getGPIO(KeyValue.c_str()));
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   CSerialComm* pForwarderCom = new CSerialComm(KeyValue);
   pForwarderCom->Open();
   Key = COM_CONFIG_KEY;
   KeyValue = ForwardINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   pForwarderCom->SetParity( KeyValue.c_str() );

   string type = ForwardINI.GetValueString(SectionStr, "VALVETYPE", "temperature");
   if (type == "heat") {
       CForwarderMonitor::GetInstance()->SetValveDataType(VALVE_DATA_TYPE_HEAT);
   } else {
       CForwarderMonitor::GetInstance()->SetValveDataType(VALVE_DATA_TYPE_TEMPERATURE);
   }
   CForwarderMonitor::GetInstance()->SetCom(pForwarderCom);
   for(uint32 i = 0; i < MAX_FORWARDER_COUNT; i++)
   {
      char ForwardIDStr[MAX_COUNT_LEN] = {0};;
      sprintf(ForwardIDStr, ID_ID_FORMAT.c_str(), i);
      Key = ForwardIDStr;
      KeyValue = ForwardINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
      TrimInvalidChar(KeyValue);
      DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
      if(INVALID_KEY_VALUE == KeyValue)
      {
         break;
      }
      uint32 ForwarderID = 0;
      sscanf(KeyValue.c_str(), FORWARD_ID_FORMAT.c_str(), &ForwarderID);
      Revert((uint8*)&ForwarderID, sizeof(ForwarderID));
      CForwarderMonitor::GetInstance()->AddForwareder( ForwarderID );
   }
   SectionStr = TIME_SESSION;
   Key = TIME_START_KEY;
   int StartTime = ForwardINI.GetValueInt(SectionStr, Key, DefaultStartTime);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), StartTime);
   Key = TIME_INTERVAL_KEY;
   int IntervalTime = ForwardINI.GetValueInt(SectionStr, Key, DefaultIntervalTime);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), IntervalTime);
   CForwarderMonitor::GetInstance()->Init(StartTime, IntervalTime);

   CForwarderMonitor::GetInstance()->Start();
}


void StartGeneralHeat()
{
   //config general heat
   CINI GeneralHeatINI("meter_cfg.ini");
   string SectionStr = HEAT_SESSION;
   string Key = COM_ADDRESS_KEY;
   string KeyValue = GeneralHeatINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   CSerialComm* pGeneralHeatCom = new CSerialComm(KeyValue);
   pGeneralHeatCom->Open();
   Key = COM_CONFIG_KEY;
   KeyValue = GeneralHeatINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   pGeneralHeatCom->SetParity( KeyValue.c_str() );

   CHeatMonitor::GetInstance()->SetCom(pGeneralHeatCom);
   for(uint32 i = 0; i < MAX_HEAT_COUNT; i++)
   {
      char GeneralHeatAddressFormat[128] = {0};
      sprintf(GeneralHeatAddressFormat, ID_ID_FORMAT.c_str(), i);
      Key = GeneralHeatAddressFormat;
      DEBUG("GeneralHeatAddressFormat=%s\n", GeneralHeatAddressFormat);
      KeyValue = GeneralHeatINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
      TrimInvalidChar(KeyValue);
      DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
      if(INVALID_KEY_VALUE == KeyValue)
      {
         break;
      }
      bool Ret = true;
      uint8 GeneralHeatAddress[MACHINENAME_LEN] = {0};;
      uint8 LowByte = 0;
      uint8 HighByte = 0;
      for(uint8 i = 0; ((i+1) < KeyValue.size()) && ((i/2)<sizeof(GeneralHeatAddress)); i+=2)
      {
         if( ( false == Char2Int(KeyValue[KeyValue.size()-i-1], LowByte) ) || ( false == Char2Int(KeyValue[KeyValue.size()-i-2], HighByte) ) )
         {
            DEBUG("StartGeneralHeat()----%s is not a valid heat address\n", KeyValue.c_str());
            Ret = false;
            break;
         }
         GeneralHeatAddress[i/2] = (HighByte<<4)|LowByte;
      }
      if(Ret)
      {
         DEBUG("Heat node address:");
         PrintData(GeneralHeatAddress, sizeof(GeneralHeatAddress));
         CHeatMonitor::GetInstance()->AddGeneralHeat((uint8*)GeneralHeatAddress, sizeof(GeneralHeatAddress));
         heatCount ++;
      }
   }
   SectionStr = TIME_SESSION;
   Key = TIME_START_KEY;
   int StartTime = GeneralHeatINI.GetValueInt(SectionStr, Key, DefaultStartTime);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), StartTime);
   Key = TIME_INTERVAL_KEY;
   int IntervalTime = GeneralHeatINI.GetValueInt(SectionStr, Key, DefaultIntervalTime);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), IntervalTime);
   CHeatMonitor::GetInstance()->Init(StartTime, IntervalTime);

   CHeatMonitor::GetInstance()->Start();
}

void StartPortal()
{
   CINI GPRSINI("gprs_cfg.ini");
   string SectionStr = GPRS_SESSION ;
   string Key = COM_ADDRESS_KEY;
   string KeyValue = GPRSINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   CGprs* pGPRS = new CGprs(KeyValue);
   pGPRS->Open();
   Key = COM_CONFIG_KEY;
   KeyValue = GPRSINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());
   pGPRS->SetParity( KeyValue.c_str() );

   Key = GPRS_LOCAL_PORT;
   uint32 LocalPort = GPRSINI.GetValueInt(SectionStr, Key, 0/*default port*/);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), LocalPort);

   Key = GPRS_SERVER_IP;
   KeyValue = GPRSINI.GetValueString(SectionStr, Key, INVALID_KEY_VALUE);
   TrimInvalidChar(KeyValue);
   DEBUG("[%s]%s=%s\n", SectionStr.c_str(), Key.c_str(), KeyValue.c_str());

   Key = GPRS_SERVER_PORT;
   uint32 DestPort = GPRSINI.GetValueInt(SectionStr, Key, 0/*default port*/);
   DEBUG("[%s]%s=%d\n", SectionStr.c_str(), Key.c_str(), DestPort);
   pGPRS->SetIP(LocalPort, KeyValue.c_str(), DestPort);

   uint8 jzqid[16];
   bzero(jzqid, 16);
   sprintf((char *)jzqid, "%08d", id);

   CPortal::GetInstance()->SetJZQID(jzqid);
   CPortal::GetInstance()->SetGPRS(pGPRS);
   Key = GPRS_HEART_INTERVAL;
   int HeartInterval = GPRSINI.GetValueInt(SectionStr, Key, DefaultIntervalTime);

   CPortal::GetInstance()->Init(HeartInterval);

   CPortal::GetInstance()->Start();
}

#ifdef SECKEY
#undef SECKEY
#define SECKEY "HOST"
#else
#define SECKEY "HOST"
#endif
void StartCardHost() {
    if (access("cardhost_cfg.ini", F_OK) == -1) { DEBUG("Missing cardhost_cfg.ini\n"); return;}
    CINI ini("cardhost_cfg.ini");
    if (!ini.GetValueBool(SECKEY, "RUN", false)) {DEBUG("Forbid card host service\n"); return;}
    string name = ini.GetValueString(SECKEY, "DEVICE", "/dev/ttyS4");
    TrimInvalidChar(name);
    string cfg = ini.GetValueString(SECKEY, "DEV_CONFIG", "115200,8,1,N");
    TrimInvalidChar(cfg);
    int com = OpenCom((char *)name.c_str(), (char *)cfg.c_str(), O_RDWR | O_NOCTTY);
    if (com == -1) {
        DEBUG("Initilize %s with %s failed!\n", name.c_str(), cfg.c_str());
        return;
    }
    DEBUG("Initilize card host servcie with %s(%s)\n", name.c_str(), cfg.c_str());
    CCardHost::GetInstance()->SetCom(com);
    SetPIOCfg(getGPIO(name.c_str()), gpioattr);
    CCardHost::GetInstance()->SetGPIO(getGPIO(name.c_str()));
    CCardHost::GetInstance()->Start();
    cardhostEnabled = true;
}

#ifdef SECKEY
#undef SECKEY
#define SECKEY "VALVE"
#else
#define SECKEY "VALVE"
#endif
void StartValveMonitor() {
    if (access("valve_cfg.ini", F_OK) == -1) { DEBUG("Missing valve_cfg.ini\n"); return;}
    CINI ini("valve_cfg.ini");
    string name = ini.GetValueString(SECKEY, "DEVICE", "/dev/ttyS5");
    TrimInvalidChar(name);
    string cfg = ini.GetValueString(SECKEY, "DEV_CONFIG", "2400,8,1,N");
    TrimInvalidChar(cfg);
    int com = OpenCom((char *)name.c_str(), (char *)cfg.c_str(), O_RDWR | O_NOCTTY);
    if (com == -1) {
        DEBUG("Initilize %s with %s failed!\n", name.c_str(), cfg.c_str());
        return;
    }
    DEBUG("Initilize valve monitor servcie with %s(%s)\n", name.c_str(), cfg.c_str());
    CValveMonitor::GetInstance()->SetCom(com);

    string type = ini.GetValueString(SECKEY, "VALVETYPE", "temperature");
    if (type == "heat") {
        CValveMonitor::GetInstance()->SetValveDataType(VALVE_DATA_TYPE_HEAT);
    } else {
        CValveMonitor::GetInstance()->SetValveDataType(VALVE_DATA_TYPE_TEMPERATURE);
    }

    int startTime = ini.GetValueInt(SECKEY, "START", 12);
    int interval = ini.GetValueInt(SECKEY, "INTERVAL", 30);
    int number = ini.GetValueInt(SECKEY, "NUMBER", 0);

    CMainLoop::GetInstance()->SetValveCount(number);

    SetPIOCfg(getGPIO(name.c_str()), gpioattr);
    CValveMonitor::GetInstance()->SetValveCount(number);
    CValveMonitor::GetInstance()->SetGPIO(getGPIO(name.c_str()));
    CValveMonitor::GetInstance()->Init(startTime, interval);
    CValveMonitor::GetInstance()->Start();
}
