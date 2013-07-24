#ifndef __UTILS_H
#define __UTILS_H
#include"sysdefs.h"
#include<time.h>
const uint8 DATETIME_LEN = 7;
void PrintData( const uint8* pData, const uint32 DataLen );
void Revert(uint8* pData, uint32 DataLen);
bool IsChar(const char CharData);
bool Char2Int(const char CharData, uint8& IntData);
uint8 GetCheckSum(const uint8* pData, const uint32 DataLen);
uint8 GenerateXorParity(const uint8* pData, const uint32 DataLen);
uint16 GenerateCRC(const uint8* puchMsg, uint32 usDataLen);
uint32 CRC32(uint8 * buf, uint32 len);
uint32 CRC32File(const char * filename);
uint32 DateTime2TimeStamp(uint8* pDateTime/*ssmmhhddmmyyyy*/, uint32 DateTimeLen);
uint32 DateTime2TimeStamp(uint32 Year, uint32 Month, uint32 Day, uint32 Hour, uint32 Minute, uint32 Second);
bool TimeStamp2TimeStr(uint32 UTCTime, uint32* pDateTime, uint32 DateTimeLen);
const char* GetLocalTimeStr();
bool GetLocalTimeStamp(uint32& UTCTime);
bool GetLocalTime(tm & t, uint32 utc);
enum LightE
{
   LIGHT_FORWARD=PB16,
   LIGHT_GENERAL_HEAT=PA12,
   LIGHT_GPRS=PB18,
   LIGHT_WARNING=PE10,
   LIGHT_MAX//never delete this
};
void FlashLight(LightE Light);
void SetLight(LightE Light, bool On);
void LOG(const char* fmt, ...);
int str2hex(char * str, uint8 * data);
#endif
