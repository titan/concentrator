#ifndef __CGPRS_H
#define __CGPRS_H
#include<string>
using namespace std;
#include"sysdefs.h"
#include"CSerialComm.h"

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
     ECommError SendData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);
     ECommError SendData(uint8* pBuffer, uint32& BufferLen, int32 SendTimeOut, uint8* pAckBuffer, uint32& AckBufferLen, int32 RecTimeOut);
     ECommError ReceiveData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);
     ECommError ReceiveData(uint8* pBuffer, uint32& BufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, uint32 BufferLenPos, int32 TimeOut = -1);
   private:
     void PowerOn();

   private:
     bool m_IsConnected;
     uint32 m_SrcPort;
     char m_DestIP[IP_MAX_LEN];
     uint32 m_DestPort;
     char m_IMEI[IMEI_LEN];
};
#endif
