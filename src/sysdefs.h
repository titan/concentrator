#ifndef SYSDEFS_H
#define SYSDEFS_H
#if __OS__ == WINDOWS
#include<windows.h>
#endif
#include<stdlib.h>

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
enum Status {
   STATUS_OK,
   STATUS_CHECKING,
   STATUS_ERROR,
   STATUS_OFFLINE,
   STATUS_MAX
};

#define USERID_LEN 8

typedef struct {
    uint8 x[USERID_LEN];
} userid_t;

typedef struct {
    userid_t uid;
    uint32 fid;
    uint32 vmac;
} user_t;

inline size_t genuserkey(void * key, size_t len) {
    size_t i = 0;
	size_t seed = 131; // 31 131 1313 13131 131313 etc..
	size_t hash = 0;

    for (; i < len; i ++) {
		hash = hash * seed + ((unsigned char *) key)[i];
	}

	return hash;
}

typedef struct {
    uint8 recharge;
    uint8 sentRecharge;
    uint16 consume;
    uint16 sentConsume;
    uint16 temperature;
    uint16 sentTemperature;
    uint8 status;
    uint8 sentStatus;
} record_t;

inline size_t genrecordkey(void * key, size_t len) {
    size_t r = 0, i;
    if (len < sizeof(size_t)) {
        for (i = 0; i < len; i ++) {
            r = (r << 8) + ((unsigned char *) key)[i];
        }
        return r;
    }
    return * (size_t *) key;
}
#endif
