#include"CSerialComm.h"
#include"CLock.h"
#include"Utils.h"

#include "libs_emsys_odm.h"

#ifdef DEBUG_COMM
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

const int32 INVALID_COMM_HANDLE = -1;
const uint32 SERIAL_COM_BUFFER_SIZE = 4096;

const uint32 COM_WAIT_TIME = 2;

CSerialComm::CSerialComm():m_hComm(INVALID_COMM_HANDLE)
{
}

CSerialComm::CSerialComm(const std::string& CommName):m_hComm(INVALID_COMM_HANDLE)
                                                      ,m_CommName(CommName)
{
}

CSerialComm::~CSerialComm()
{
  Close();
}

bool CSerialComm::Open()
{
   DEBUG("CSerialComm::Open()\n");
   if(IsOpen())
   {
      return true;
   }
   return Open(m_CommName);
}

bool CSerialComm::Open( const std::string& CommName )
{
  Close();

  m_CommName = CommName;
  const char* DEFCONFIG = "115200,8,1,N";
  DEBUG("%s\n", CommName.c_str());
  m_hComm = OpenCom( (char*)(CommName.c_str()), (char*)DEFCONFIG, O_RDWR|O_NOCTTY);
  if( m_hComm == INVALID_COMM_HANDLE )
  {
    return false;
  }

  return true;
}

bool CSerialComm::SetParity(const char* pConfigStr)
{
  if( (NULL == pConfigStr) || (false == IsOpen()) )
  {
     return false;
  }

  DEBUG("Config=%s\n", pConfigStr);
  if(SetComCfg(m_hComm, (char*)pConfigStr) != 0)
  {
    DEBUG("Config Serial Port: %s error!\n", pConfigStr);
    Close();
    return false;
  }

  return true;
}

bool CSerialComm::SetParity(EBaudRate BaudRate, EDataBits Databits, EStopBits Stopbits, EParity Parity)
{
  if( false == IsOpen() )
  {
     return false;
  }

  const char* BaudRateStr[] =
  {
     "110", "300", "600", "1200", "2400", "4800", "9600", "19200", "38400", "57600", "115200", "230400", "460800", "921600"
  };
  const char DataBitChar[] = {'5', '6', '7', '8'};
  const char StopbitsChar[] = {'1', '2'};
  const char ParityChar[] = {'N', 'O', 'E', 'S', 'M'};

  string ComFig = BaudRateStr[BaudRate];
  ComFig += ',';
  ComFig += DataBitChar[Databits];
  ComFig += ',';
  ComFig += StopbitsChar[Stopbits];
  ComFig += ',';
  ComFig += ParityChar[Parity];

  DEBUG("Config=%s\n", ComFig.c_str());
  if(SetComCfg(m_hComm, (char*)ComFig.c_str()) != 0)
  {
    DEBUG("Config Serial Port: %s error!\n", ComFig.c_str());
    Close();
    return false;
  }

  return true;
}

bool CSerialComm::IsOpen() const
{
  return INVALID_COMM_HANDLE != m_hComm;
}

void CSerialComm::Close()
{
  if(INVALID_COMM_HANDLE == m_hComm)
  {
    return;
  }

  CloseCom(m_hComm);
  m_hComm = INVALID_COMM_HANDLE;
  m_CommName = "";
}

//*                     : <timesout>[in]   指定最大的阻塞读取等待间隔，单位: useconds(微秒)
//*                     :        当timesout<=0，阻塞读模式，即直到读够指定数据个数后函数返回
ECommError CSerialComm::WriteBuf(uint8* pBuffer, uint32& BufferLen, int32 TimeOut)
{
  DEBUG("CSerialComm::WriteBuf()\n");
  PrintData(pBuffer, BufferLen);

  //write data
  uint32 TotalWritedBytes = 0;
  uint32 WriteCount = 0;
  while(TotalWritedBytes < BufferLen)
  {
    uint32 WritedBytes = BufferLen-TotalWritedBytes;
    int32 Ret = WriteCom(m_hComm, (char*)(pBuffer+TotalWritedBytes), &WritedBytes, TimeOut);
    DEBUG("fd: 0x%x Ret=%d, WritedBytes=%d\n", m_hComm, Ret, WritedBytes);
    if( (-1 == Ret) || (WriteCount > MAX_WRITEREAD_COUNT) )
    {
      return COMM_FAIL;
    }
    WriteCount++;
    TotalWritedBytes += WritedBytes;
  }
  return COMM_OK;
}

ECommError CSerialComm::ReadBuf(uint8* pBuffer, uint32& BufferLen, int32 TimeOut)
{
   DEBUG("BufferLen(read)=%d\n", m_hComm, BufferLen);
   int32 Ret = ReadCom(m_hComm, (char*)pBuffer, &BufferLen, TimeOut);
   DEBUG("Ret=%d, BufferLen=%d\n", m_hComm, Ret, BufferLen);
   if(Ret != 0)
   {
      return COMM_FAIL;
   }
   PrintData(pBuffer, BufferLen);
   return COMM_OK;
}

ECommError CSerialComm::ReadBuf(uint8* pBuffer, uint32& BufferLen, const uint8 BeginFlag, const uint8 EndFlag, int32 TimeOut)
{
   return ReadBuf(pBuffer, BufferLen, &BeginFlag, sizeof(BeginFlag), &EndFlag, sizeof(EndFlag), TimeOut);;
}

ECommError CSerialComm::ReadBuf(uint8* pBuffer, uint32& BufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, const uint8* pEndFlag, uint32 EndFlagLen, int32 TimeOut)
{
   DEBUG("CSerialComm::ReadBuf(0x%x)\n", m_hComm);
   //flush read Buffer
   tcflush(m_hComm,TCIFLUSH);
   //find the begin flag
   uint8 ReadBuffer[MAX_BUFFER_LEN]={0};
   int32 BeginFlagIndex = -1;
   int32 EndFlagIndex = -1;

   uint32 nTotalReadBytes = 0;
   uint8 i = 0;
   for(; i < MAX_WRITEREAD_COUNT; i++)
   {
      uint32 nReadBytes = sizeof(ReadBuffer);
      ECommError Ret = ReadBuf(ReadBuffer+nTotalReadBytes, nReadBytes, TimeOut);
      DEBUG("fd: 0x%x Retry %d-Result = %d, readbytes=%d-nTotalReadBytes=%d\n", m_hComm, i, Ret, nReadBytes, nTotalReadBytes);
      if(COMM_OK != Ret)
      {
         return Ret;
      }
      nTotalReadBytes += nReadBytes;

      //find the BeginFlag
      if( pBeginFlag && (-1 == BeginFlagIndex) && (nTotalReadBytes >= BeginFlagLen)  )
      {
         uint32 Index = 0;
         for(; Index < (nTotalReadBytes-BeginFlagLen); Index++)
         {
            if(0 == memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen) )
            {
               break;
            }
         }
         if(memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen))
         {
            continue;
         }
         BeginFlagIndex = Index;
         i = 0;//reset index becasue there should be data coming
         DEBUG("fd:0x%x Find BeginFlag  BeginFlagIndex=%d\n", m_hComm, BeginFlagIndex);
      }
      //find the EndFlag
      if( pEndFlag && (-1 != BeginFlagIndex) && (-1 == EndFlagIndex) && (nTotalReadBytes >= EndFlagLen) )
      {
         uint32 Index = nTotalReadBytes-EndFlagLen;
         for(; Index >= BeginFlagIndex+BeginFlagLen; Index-- )
         {
            if(0 == memcmp(ReadBuffer+Index, pEndFlag, EndFlagLen) )
            {
               break;
            }
         }
         if(memcmp(ReadBuffer+Index, pEndFlag, EndFlagLen))
         {
            sleep( (i+1)*COM_WAIT_TIME );//wait data to come because there should be data to come
            continue;
         }
         EndFlagIndex = Index;
         DEBUG("fd:0x%x Find EndFlag  EndFlagIndex=%d\n", m_hComm, EndFlagIndex);
         break;
      }
   }
   if(i >= MAX_WRITEREAD_COUNT)
   {
      DEBUG("fd: 0x%x Can't read valid data\n", m_hComm);
      return COMM_NODATA;
   }

   //copy buffer
   uint32 TotalReadBytesCount = EndFlagIndex-BeginFlagIndex+EndFlagLen;
   if( TotalReadBytesCount > BufferLen )
   {
      DEBUG("fd: 0x%x Buffer NOT enough, %d bytes needed, memory size=%d\n", m_hComm, TotalReadBytesCount, BufferLen);
      return COMM_MEMORY_NOT_ENOUGH;
   }
   memcpy(pBuffer, ReadBuffer+BeginFlagIndex, TotalReadBytesCount);
   BufferLen = TotalReadBytesCount;

   return COMM_OK;
}

ECommError CSerialComm::ReadMinByte(uint8* pBuffer, uint32& BufferLen, const uint32 MinBufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, const uint8* pEndFlag, uint32 EndFlagLen, int32 TimeOut)
{
   DEBUG("fd: 0x%x\n", m_hComm);
   //flush read Buffer
   tcflush(m_hComm,TCIFLUSH);
   //find the begin flag
   uint8 ReadBuffer[MAX_BUFFER_LEN]={0};
   int32 BeginFlagIndex = -1;
   int32 EndFlagIndex = -1;

   uint32 nTotalReadBytes = 0;
   uint8 i = 0;
   for(; i<MAX_WRITEREAD_COUNT; i++)
   {
      uint32 nReadBytes = sizeof(ReadBuffer);
      ECommError Ret = COMM_OK;
      uint32 j = 0;
      for(; j < MAX_WRITEREAD_COUNT; j++)
      {
         Ret = ReadBuf(ReadBuffer+nTotalReadBytes, nReadBytes, TimeOut);
         DEBUG("fd: 0x%x Retry %d-read j=%d Result = %d, readbytes=%d-nTotalReadBytes=%d\n", m_hComm, i, j, Ret, nReadBytes, nTotalReadBytes);
         if(COMM_OK != Ret)
         {
            return Ret;
         }
         nTotalReadBytes += nReadBytes;
         if(nTotalReadBytes >= MinBufferLen)
         {
            break;
         }
         myusleep(100 * 1000);
      }
      if(j >= MAX_WRITEREAD_COUNT)
      {
         continue;
      }

      //find the BeginFlag
      if( pBeginFlag && (-1 == BeginFlagIndex) && (nTotalReadBytes >= BeginFlagLen)  )
      {
         uint32 Index = 0;
         for(; Index < (nTotalReadBytes-BeginFlagLen); Index++)
         {
            if(0 == memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen) )
            {
               break;
            }
         }
         if(memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen))
         {
            continue;
         }
         BeginFlagIndex = Index;
         DEBUG("fd: 0x%x Find BeginFlag  BeginFlagIndex=%d\n", m_hComm, BeginFlagIndex);
      }
      //find the EndFlag
      if( pEndFlag && (-1 != BeginFlagIndex) && (-1 == EndFlagIndex) && (nTotalReadBytes >= EndFlagLen) )
      {
         uint32 Index = nTotalReadBytes-EndFlagLen;
         for(; Index >= BeginFlagIndex+BeginFlagLen; Index-- )
         {
            if(0 == memcmp(ReadBuffer+Index, pEndFlag, EndFlagLen) )
            {
               break;
            }
         }
         if(memcmp(ReadBuffer+Index, pEndFlag, EndFlagLen))
         {
            continue;
         }
         EndFlagIndex = Index;
         DEBUG("fd: 0x%x Find EndFlag  EndFlagIndex=%d\n", m_hComm, EndFlagIndex);
         break;
      }
   }
   if(i >= MAX_WRITEREAD_COUNT)
   {
      DEBUG("fd: 0x%x Can't read valid data\n", m_hComm);
      return COMM_NODATA;
   }

   //copy buffer
   uint32 TotalReadBytesCount = EndFlagIndex-BeginFlagIndex+EndFlagLen;
   if( TotalReadBytesCount > BufferLen )
   {
      DEBUG("fd: 0x%x Buffer NOT enough, %d bytes needed, memory size=%d\n", m_hComm, TotalReadBytesCount, BufferLen);
      return COMM_MEMORY_NOT_ENOUGH;
   }
   memcpy(pBuffer, ReadBuffer+BeginFlagIndex, TotalReadBytesCount);
   BufferLen = TotalReadBytesCount;

   return COMM_OK;
}

ECommError CSerialComm::ReadMinByte(uint8* pBuffer, uint32& BufferLen, const uint32 MinBufferLen, const uint8 BeginFlag, const uint8 EndFlag, int32 TimeOut)
{
   return ReadMinByte(pBuffer, BufferLen, MinBufferLen, &BeginFlag, sizeof(BeginFlag), &EndFlag, sizeof(EndFlag), TimeOut);;
}
