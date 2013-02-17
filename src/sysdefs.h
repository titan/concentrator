#ifndef SYSDEFS_H
#define SYSDEFS_H
#if __OS__ == WINDOWS
#include<windows.h>
#endif

// #define CHINA_MOBILE
#define CHINA_UNION
typedef signed char    		int8;
typedef signed short   		int16;
typedef int int32;
typedef signed long long int64;

typedef unsigned char  		uint8;
typedef unsigned short 		uint16;
typedef unsigned int  		uint32;
typedef unsigned long long  uint64;


const uint8 VERSION = 1;
const uint8 SUBVERSION = 15;

#define INVALID_DATA (-1)
const uint32 ONE_SECOND = 1000*1000;//ms

const uint8 GENERAL_HEAT_DATA_PACKET_LEN = 46;
const uint8 FORWARDER_TYPE_HEAT_DATA_LEN = 45;
const uint8 FORWARDER_TYPE_TEMPERATURE_DATA_LEN = 47;
const uint8 FORWARDER_CHARGE_PACKET_LEN = 56;
const uint8 FORWARDER_CONSUME_PACKET_LEN = 42;

const uint8 MAX_RETRY_COUNT = 3;
const uint32 MAX_BUFFER_LEN = 1024;
const uint32 MAX_PACKET_LEN = 2048;
enum ErrorCodeE
{
   ERROR_OK = 0,
   ERROR_USER_CANCEL,
   ERROR_TIMER_INIT_FAIL,
   ERROR_TIMER_SET_TIMER_FAIL,
   ERROR_COMM,
   ERROR_COUNT//never delete this
};
enum StatusE
{
   STATUS_OK,
   STATUS_CHECKING,
   STATUS_ERROR,
   STATUS_OFFLINE,
   STATUS_MAX
};
#endif
