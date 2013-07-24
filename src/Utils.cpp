#include<stdio.h>
#include<assert.h>
#include<stdarg.h>
#include"CLock.h"
#include"Utils.h"
#include"libs_emsys_odm.h"
#include "CRC32.h"

#ifdef DEBUG_UTILS
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

void PrintData( const uint8* pData, const uint32 DataLen )
{
   assert(pData && DataLen>0);
   if(NULL == pData || 0 >= DataLen)
   {
      return;
   }
   DEBUG("<--");
   uint32 i = 0;
   for(; i<DataLen; i++)
   {
      DEBUG("0x%02x ", pData[i]);
   }
   DEBUG("-->\n");
}

void Revert(uint8* pData, uint32 DataLen)
{
   assert(pData && DataLen>0);
   if(NULL == pData || 0 >= DataLen)
   {
      return;
   }
   uint8* pBegin = pData;
   uint8* pEnd = pData+DataLen-1;
   uint8 Temp = 0;
   while(pBegin<pEnd)
   {
      Temp = *pBegin;
      *pBegin = *pEnd;
      *pEnd = Temp;
      pBegin++;
      pEnd--;
   }
}

bool IsChar(const char CharData)
{
   if((CharData>='0') && (CharData<='9'))
   {
      return true;
   }
   if((CharData>='a') && (CharData<='z'))
   {
      return true;
   }
   if((CharData>='A') && (CharData<='Z'))
   {
      return true;
   }

   const char ValidCharStr[]={'/', '\\'};
   for(uint8 i = 0; i < sizeof(ValidCharStr); i++)
   {
      if(ValidCharStr[i] == CharData)
      {
         return true;
      }
   }

   return false;
}
uint8 GetCheckSum(const uint8* pData, const uint32 DataLen)
{
   assert(pData && DataLen>0);
   if(NULL == pData || 0 >= DataLen)
   {
      return 0xFF;
   }
   uint8 CheckSum = 0;
   for(uint32 i = 0; i < DataLen; i++)
   {
      CheckSum += pData[i];
   }
   return CheckSum;
}

bool Char2Int(const char CharData, uint8& IntData)
{
   if((CharData>='0') && (CharData<='9'))
   {
      IntData = CharData-'0';
      return true;
   }
   if((CharData>='a') && (CharData<='f'))
   {
      IntData = CharData-'a'+10;
      return true;
   }
   if((CharData>='A') && (CharData<='F'))
   {
      IntData = CharData-'A'+10;
      return true;
   }
   return false;
}

uint8 GenerateXorParity(const uint8* pData, const uint32 DataLen)
{
   assert(pData && DataLen>0);
	uint8 xor_tmp=0;
	for(uint8 i=0; i<DataLen;i++)
   {
		xor_tmp ^= pData[i];
	}
	return xor_tmp;
}

uint16 GenerateCRC(const uint8* puchMsg, uint32 usDataLen)
{
   static unsigned char auchCRCHi[] =
   {
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
      0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
      0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
      0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
      0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
   } ;
   /* CRC 低位字节值表*/
   static unsigned char auchCRCLo[] =
   {
      0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
      0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
      0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
      0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
      0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
      0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
      0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
      0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
      0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
      0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
      0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
      0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
      0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
      0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
      0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
      0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
      0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
      0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
      0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
      0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
      0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
      0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
      0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
      0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
      0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
      0x43, 0x83, 0x41, 0x81, 0x80, 0x40
   } ;

   uint8 uchCRCHi = 0xFF ; /* 高CRC 字节初始化*/
   uint8 uchCRCLo = 0xFF ; /* 低CRC 字节初始化*/
   uint16 uIndex ; /* CRC 循环中的索引*/
   while (usDataLen--) /* 传输消息缓冲区*/
   {
      uIndex = uchCRCHi ^ *puchMsg++ ; /* 计算CRC */
      uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex] ;
      uchCRCLo = auchCRCLo[uIndex] ;
   }
   return (uchCRCHi << 8 | uchCRCLo) ;
}

uint32 CRC32(uint8 * buf, uint32 len) {
    Crc32 crc32;
    crc32.AddData(buf, len);
    return crc32.GetCrc32();
}

uint32 CRC32File(const char * filename) {
    int fd = open(filename, O_RDONLY);
    if (fd != -1) {
        Crc32 crc32;
        ssize_t len;
        uint8 buf[4096];
        bzero(buf, 4096);
        while ((len = read(fd, buf, 4096)) > 0) {
            crc32.AddData(buf, len);
            bzero(buf, 4096);
        }
        return crc32.GetCrc32();
    } else {
        return 0;
    }
}

uint32 DateTime2TimeStamp(uint8* pDateTime, uint32 DateTimeLen)
{
   assert(DateTimeLen == 7);
   uint8 Second = (pDateTime[0]&0xF0)>>4;
   Second = Second*10 + (pDateTime[0]&0x0F);

   uint8 Minute = (pDateTime[1]&0xF0)>>4;
   Minute = Minute*10 + (pDateTime[1]&0x0F);

   uint8 Hour = (pDateTime[2]&0xF0)>>4;
   Hour = Hour*10 + (pDateTime[2]&0x0F);

   uint8 Day = (pDateTime[3]&0xF0)>>4;
   Day = Day*10 + (pDateTime[3]&0x0F);

   uint8 Month = (pDateTime[4]&0xF0)>>4;
   Month = Month*10 + (pDateTime[4]&0x0F);

   uint16 Year = (pDateTime[6]&0xF0)>>4;
   Year = Year*10 + (pDateTime[6]&0x0F);
   Year = Year*10 + ((pDateTime[5]&0xF0)>>4);
   Year = Year*10 + (pDateTime[5]&0x0F);
   return DateTime2TimeStamp(Year, Month, Day, Hour, Minute, Second);
}

#ifndef _TM_NEW_DEFINED
typedef  struct _tm_new
{
    int	tm_sec;       /* second 0-59 */
    int	tm_min;       /* minute 0-59 */
    int	tm_hour;      /* hour   0-23 */
    int	tm_mday;      /* day of month  1-31 */
    int	tm_mon;       /* month  0-11 */
    int	tm_year;      /* year from 1900, Examples:2009 --> 109 */
    int	tm_wday;      /* week 0-6, sunday is 0 */
    int	tm_yday;      /* day of year 0-365 */
    int	tm_isdst;     /* daylight saving time.*/
}tm_new;
#define _TM_NEW_DEFINED
#endif
#define SECSPERMIN	    60L
#define MINSPERHOUR	    60L
#define HOURSPERDAY	    24L
#define SECSPERHOUR	    (SECSPERMIN * MINSPERHOUR)
#define SECSPERDAY	     (SECSPERHOUR * HOURSPERDAY)
#define DAYSPERWEEK	    7
#define MONSPERYEAR	    12

#define LOCALTIMEAREA    8
#define LOCALTIMEADJ   (LOCALTIMEAREA*SECSPERMIN*MINSPERHOUR)

#define _SEC_IN_MINUTE  60
#define _SEC_IN_HOUR    3600
#define _SEC_IN_DAY     86400

#define YEAR_BASE	    1900
#define EPOCH_YEAR      1970
#define EPOCH_WDAY      4

/* Judge leap year, If true,return 1; else return 0 */
#define isleap(y)       ((((y) % 4) == 0 && ((y) % 100) != 0) || ((y) % 400) == 0)

static const unsigned char mon_lengths[2][MONSPERYEAR] =
{
    {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
    {31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
} ;

static const unsigned short year_lengths[2] =
{
    365,
    366
} ;
tm_new local_time;
unsigned int UTCTime;

void gmtime_new(tm_new * res,const unsigned long timestamp)
{
    unsigned long days, rem,temp_d;
    unsigned int y;
    unsigned int yleap;
    const unsigned char * ip;

    days = timestamp / SECSPERDAY; //得到天
    rem  = timestamp % SECSPERDAY; //得到天之后秒数

    /* compute hour, min, and sec */
    temp_d = (rem / SECSPERHOUR);
    res->tm_hour=temp_d;
    temp_d = rem % SECSPERHOUR;
    rem = temp_d;
    res->tm_min =  (rem / SECSPERMIN);
    res->tm_sec =  (rem % SECSPERMIN);

    /* compute day of week */
    if ((res->tm_wday = ((EPOCH_WDAY + days) % DAYSPERWEEK)) < 0)
        res->tm_wday += DAYSPERWEEK;


    /* compute year & day of year */
    y = EPOCH_YEAR;
    for (;;)
    {
        yleap = isleap(y);
        if (days < year_lengths[yleap])
        {
            break;
        }
        y++;
        days -= year_lengths[yleap];
    }

    res->tm_year = y - YEAR_BASE;
    res->tm_yday = days;
    ip = mon_lengths[yleap];
    for (res->tm_mon = 0; days >= ip[res->tm_mon]; ++res->tm_mon)
    {
        days -= ip[res->tm_mon];
    }
    res->tm_mday = days + 1;

    /* set daylight saving time flag */
    res->tm_isdst = -1;

}

static const unsigned short _DAYS_BEFORE_MONTH[12] =
    {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

#define _ISLEAP(y) (((y) % 4) == 0 && (((y) % 100) != 0 || (((y)+1900) % 400) == 0))
#define _DAYS_IN_YEAR(year) (_ISLEAP(year) ? 366 : 365)

uint32 mktime_new( tm_new * tim_p)
{
    unsigned long tim=0;
    unsigned long days = 0;
    long year;

    /* compute hours, minutes, seconds */
    tim += tim_p->tm_sec + (tim_p->tm_min * _SEC_IN_MINUTE) +
           ((tim_p->tm_hour) * _SEC_IN_HOUR);

    /* compute days in year */
    days += tim_p->tm_mday - 1;
    days += _DAYS_BEFORE_MONTH[tim_p->tm_mon];
    if (tim_p->tm_mon > 1 && _DAYS_IN_YEAR (tim_p->tm_year) == 366)
        days++;

    /* compute days in other years */
    for (year = 70; year < tim_p->tm_year; year++)
        days += _DAYS_IN_YEAR (year);


    /* compute total seconds */
    tim += (days * _SEC_IN_DAY);

    return tim;
}


uint32 DateTime2TimeStamp(uint32 Year, uint32 Month, uint32 Day, uint32 Hour, uint32 Minute, uint32 Second)
{
   uint8 DateTimeStr[128] = {0};
   sprintf((char*)DateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", Year, Month, Day, Hour, Minute, Second);
   DEBUG("DateTimeStr=%s\n", DateTimeStr);
   // tm_new tm;
   tm TmTime;
   TmTime.tm_sec = Second;
   TmTime.tm_min = Minute;
   TmTime.tm_hour = Hour;
   TmTime.tm_mday = Day;
   TmTime.tm_mon = Month-1;
   TmTime.tm_year = Year-1900;
   return mktime(&TmTime);
   // return mktime_new(&tm);
}

bool TimeStamp2TimeStr(uint32 UTCTime, uint32* pDateTime, uint32 DateTimeLen)
{
   DEBUG("DateTimeLen=%d\n", DateTimeLen);
   if( (NULL == pDateTime) || (DATETIME_LEN != DateTimeLen) )
   {
      assert(0);
      return false;
   }
   UTCTime += 8*60*60;//converted to be Beijing Time
   time_t t = (const time_t)(UTCTime);
   tm* pTmTime = gmtime(&t);
   pDateTime[0] = pTmTime->tm_year+1900;
   pDateTime[1] = pTmTime->tm_mon+1;//tm:month from 0-11, RTC:month from 1-12
   pDateTime[2] = pTmTime->tm_mday;
   pDateTime[3] = pTmTime->tm_hour;
   pDateTime[4] = pTmTime->tm_min;
   pDateTime[5] = pTmTime->tm_sec;

   return true;
}

const char* GetLocalTimeStr()
{
   static char DateTimeStr[128] = {0};
   uint32 TimeData[7] = {0};
   if(ERROR_OK != GetDateTime(TimeData, 1))//RTC
   {
      DEBUG("Can't read time from RTC\n");
      return NULL;
   }
   sprintf(DateTimeStr, "%04d-%02d-%02d %02d:%02d:%02d", TimeData[0], TimeData[1], TimeData[2], TimeData[3], TimeData[4], TimeData[5]);
   return DateTimeStr;
}

bool GetLocalTimeStamp(uint32& UTCTime)
{
   uint32 TimeData[DATETIME_LEN] = {0};
   if(ERROR_OK != GetDateTime(TimeData, 1))//RTC
   {
      DEBUG("Can't read time from RTC\n");
      return false;
   }
   tm tm_CurrentTime;
   tm_CurrentTime.tm_year = TimeData[0]-1900;
   tm_CurrentTime.tm_mon = TimeData[1]-1;
   tm_CurrentTime.tm_mday = TimeData[2];
   tm_CurrentTime.tm_hour = TimeData[3];
   tm_CurrentTime.tm_min = TimeData[4];
   tm_CurrentTime.tm_sec = TimeData[5];
   time_t CurrentTime = mktime(&tm_CurrentTime);
   if(-1 == CurrentTime)
   {
      DEBUG("NOT invalid calender-year can't be over 1900+137--%04d-%02d-%02d %02d:%02d:%02d\n", TimeData[0], TimeData[1], TimeData[2], TimeData[3], TimeData[4], TimeData[5]);
      return false;
   }
   UTCTime = (uint32)CurrentTime;

   return true;
}

bool GetLocalTime(tm & t, uint32 utc) {
    tm_new tn;
    gmtime_new(&tn, utc);
    t.tm_year = tn.tm_year;
    t.tm_mon = tn.tm_mon;
    t.tm_mday = tn.tm_mday;
    t.tm_hour = tn.tm_hour;
    t.tm_min = tn.tm_min;
    t.tm_sec = tn.tm_sec;
    t.tm_wday = tn.tm_wday;
    return true;
}

void FlashLight(LightE Light)
{
   uint32 GPIOPin = Light;
   gpio_attr_t AttrOut;
   AttrOut.mode = PIO_MODE_OUT;
   AttrOut.resis = PIO_RESISTOR_DOWN;
   AttrOut.filter = PIO_FILTER_NOEFFECT;
   AttrOut.multer = PIO_MULDRIVER_NOEFFECT;
   SetPIOCfg((gpio_name_t)GPIOPin, AttrOut);

   PIOOutValue((gpio_name_t)GPIOPin, 1);
   myusleep(100 * 1000);
   PIOOutValue((gpio_name_t)GPIOPin, 0);
}

void SetLight(LightE Light, bool On)
{
   uint32 GPIOPin = Light;
   gpio_attr_t AttrOut;
   AttrOut.mode = PIO_MODE_OUT;
   AttrOut.resis = PIO_RESISTOR_DOWN;
   AttrOut.filter = PIO_FILTER_NOEFFECT;
   AttrOut.multer = PIO_MULDRIVER_NOEFFECT;
   SetPIOCfg((gpio_name_t)GPIOPin, AttrOut);

   PIOOutValue((gpio_name_t)GPIOPin, On);
}

void LOG(const char* fmt, ...)
{
   static CLock LogLock;
   LogLock.Lock();
   static FILE* pFile = fopen("importantLog.log", "a+");
   if(NULL == pFile)
   {
      pFile = fopen("importantLog.log", "a+");
   }
   if(NULL == pFile)
   {
      LogLock.UnLock();
      return;
   }
   char LOGStr[2048] = {0};
   time_t NowTime;
   tm *Local = localtime(&NowTime);
   strcpy(LOGStr, asctime(Local));
   LOGStr[strlen(LOGStr)-1] = ':';

   va_list args;
   va_start(args,fmt);
   vsprintf(LOGStr+strlen(LOGStr),fmt,args);
   va_end(args);

   fputs(LOGStr, pFile);
   fflush(pFile);

   LogLock.UnLock();
}

int str2hex(char * str, uint8 * data) {
    uint8 tmp = 0;
    int k = 0;
    while (str[0] == '0' && str[0] != 0) str ++;
    for (int i = 0, len = strlen(str), j = len % 2; i < len; i ++) {
        switch (str[i]) {
        case '1':
            tmp |= 0x1;
                break;
        case '2':
            tmp |= 0x2;
            break;
        case '3':
            tmp |= 0x3;
            break;
        case '4':
            tmp |= 0x4;
            break;
        case '5':
            tmp |= 0x5;
            break;
        case '6':
            tmp |= 0x6;
            break;
        case '7':
            tmp |= 0x7;
            break;
        case '8':
            tmp |= 0x8;
            break;
        case '9':
            tmp |= 0x9;
            break;
        case 'a':
        case 'A':
            tmp |= 0xa;
            break;
        case 'b':
        case 'B':
            tmp |= 0xb;
            break;
        case 'c':
        case 'C':
            tmp |= 0xc;
            break;
        case 'd':
        case 'D':
            tmp |= 0xd;
            break;
        case 'e':
        case 'E':
            tmp |= 0xe;
            break;
        case 'f':
        case 'F':
            tmp |= 0xf;
            break;
        default:
            break;
        }
        if (i % 2 == j) {
            tmp = tmp << 4;
        } else {
            data[k++] = tmp;
            tmp = 0;
        }
    }
    return k;
}
