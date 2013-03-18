#ifndef __CGPRS_H
#define __CGPRS_H
#include<string>
using namespace std;
#include"sysdefs.h"
#include"CSerialComm.h"

#define LINE_LEN        54
#define LINE_REAL_LEN   64
#define RX_TIMEOUT      450 // ms
#define SIM_TIMEOUT     4005 // ms
#define NETREG_TIMEOUT  20007 // ms
#define SMS_TIMEOUT     18000 // ms

enum GPRSWorkMode {
    WORK_MODE_TT = 0, // Transparent transmission
    WORK_MODE_HTTP
};

const uint8 UNKNOWN_SIGNAL_INTESITY = 99;
const uint8 IP_MAX_LEN = 128;
const uint8 IMEI_LEN = 15;
const uint32 MAX_GPRS_PACKET_LEN = 1024;
const uint32 GPRS_PACKET_LEN_LEN = 2;//each GPRS packet length is 2 bytes
class CGprs:public CSerialComm
{
   public:
     CGprs();
     CGprs(const string& CommName);
     CGprs(const string& CommName, uint32 SrcPort, char* pDestIPStr, uint32 DestPort);
     void SetIP(uint32 SrcPort, const char* pDestIPStr, uint32 DestPort);
     bool Connect();
     bool Connect(const char* DestIP, const uint32 DestPort);
     bool IsConnected(){return m_IsConnected;};
     bool GetSignalIntesity(uint8& nSignalIntesity);
     void Disconnect();
     bool GetIMEI(uint8* pIMEI, uint32& IMEILen);
//*: <timesout>[in]   指定最大的阻塞读取等待间隔，单位: useconds(微秒)
//*:当timesout<=0，阻塞读模式，即直到读够指定数据个数后函数返回
     ECommError HttpGet(const char * url, uint8 * buf, uint32 * len);
     ECommError SendData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);
     ECommError SendData(uint8* pBuffer, uint32& BufferLen, int32 SendTimeOut, uint8* pAckBuffer, uint32& AckBufferLen, int32 RecTimeOut);
     ECommError ReceiveData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);
     ECommError ReceiveData(uint8* pBuffer, uint32& BufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, uint32 BufferLenPos, int32 TimeOut = -1);
   private:
     void PowerOn();
     bool SwitchMode(enum GPRSWorkMode to);
     ECommError Command(const char * cmd, int32 timeout, const char * reply, char * buf, uint32 & len);
     ECommError Command(const char * cmd, int32 timeout, const char * reply) { char buf[LINE_LEN + 4]; uint32 len = LINE_LEN; bzero(buf, LINE_LEN + 4); return Command(cmd, timeout, reply, buf, len); };
     ECommError Command(const char * cmd, int32 timeout) { return Command(cmd, timeout, "OK"); }
     ECommError Command(const char * cmd, const char * reply) { return Command(cmd, RX_TIMEOUT, reply); }
     ECommError Command(const char * cmd) { return Command(cmd, "OK"); }
     ECommError WaitString(const char * str, int32 timeout, char * buf, uint32 & len);
     ECommError WaitString(const char * str, int32 timeout) { char buf[LINE_LEN + 4]; uint32 len = LINE_LEN; bzero(buf, LINE_LEN + 4); return WaitString(str, timeout, buf, len); };
     ECommError WaitAnyString(int32 timeout, char * buf, uint32 & len);
     ECommError ReadRawData(uint8 * buf, uint32 len, int32 timeout);
     void Delay(uint32 ms);
   private:
     bool m_IsConnected;
     uint32 m_SrcPort;
     char m_DestIP[IP_MAX_LEN];
     uint32 m_DestPort;
     char m_IMEI[IMEI_LEN];
     enum GPRSWorkMode mode;
};
#endif
