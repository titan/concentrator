#ifndef __CSERIALCOMM_H
#define __CSERIALCOMM_H

#include<string>
#include"sysdefs.h"
using namespace std;
enum EBaudRate
{
   BR_110 = 0,
   BR_300,
   BR_600,
   BR_1200,
   BR_2400,
   BR_4800,
   BR_9600,
   BR_19200,
   BR_38400,
   BR_57600,
   BR_115200,
   BR_230400,
   BR_460800,
   BR_921600,
   BR_MAX//never delete this
};
enum ECommError
{
   COMM_OK,
   COMM_FAIL,
   COMM_NODATA,
   COMM_INVALID_DATA,
   COMM_MEMORY_NOT_ENOUGH,
   COMM_ERROR_MAX//never delete this
};
enum EDataBits
{
   BITS_5=0,
   BITS_6,
   BITS_7,
   BITS_8,
   BITS_MAX//never delete this
};
enum EStopBits
{
   STOPBITS_1=0,
   STOPBITS_2,
   STOPBITS_MAX//never delete this
};
enum EParity
{
   PARITY_CLEAR=0,
   PARITY_OOD,
   PARITY_EVEN,
   PARITY_SPACE,
   PARITY_MARK,
   PARITY_MAX//never delete this
};
const uint32 COMM_ERROR = -1;
const uint32 MAX_WRITEREAD_COUNT = 3;
class CSerialComm
{
   public:
      CSerialComm();
      CSerialComm(const std::string& CommName);
      ~CSerialComm();
      virtual bool Open();//default com config "115200,8,1,N"
      virtual bool Open(const std::string& CommName);

      bool SetParity(const char* pConfigStr);
      bool SetParity(EBaudRate BaudRate, EDataBits Databits, EStopBits Stopbits, EParity Parity);
      bool IsOpen() const;
      virtual void Close();

//*                     :<timesout>[in]   指定最大的阻塞读取等待间隔，单位: useconds(微秒)
//*                     :当timesout<=0，阻塞读模式，即直到读够指定数据个数后函数返回
      ECommError WriteBuf(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);

//*                     :<timesout>[in]   指定最大的阻塞读取等待间隔，单位: useconds(微秒)
//*                     :当timesout<=0，阻塞读模式，即直到读够指定数据个数后函数返回
      ECommError ReadBuf(uint8* pBuffer, uint32& BufferLen, int32 TimeOut = -1);
      ECommError ReadBuf(uint8* pBuffer, uint32& BufferLen, const uint8 BeginFlag, const uint8 EndFlag, int32 TimeOut = -1);
      ECommError ReadBuf(uint8* pBuffer, uint32& BufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, const uint8* pEndFlag, uint32 EndFlagLen, int32 TimeOut = -1);
      ECommError ReadMinByte(uint8* pBuffer, uint32& BufferLen, const uint32 MinBufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, const uint8* pEndFlag, uint32 EndFlagLen, int32 TimeOut = -1);
      ECommError ReadMinByte(uint8* pBuffer, uint32& BufferLen, const uint32 MinBufferLen, const uint8 BeginFlag, const uint8 EndFlag, int32 TimeOut = -1);
   protected:
      int32 m_hComm;
      std::string m_CommName;
};
void WriteLog(const char *fmt, ...);
#define LOGTRACE                   WriteLog

#endif
