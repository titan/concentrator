#include<assert.h>
#include<string>
using namespace std;

#include"CGprs.h"

#ifndef CFG_TIMER_LIB
#define CFG_TIMER_LIB
#endif
#ifndef CFG_GPIO_LIB
#define CFG_GPIO_LIB 1
#endif
#include"libs_emsys_odm.h"

#ifdef DEBUG_GPRS
#define DEBUG(...) do {printf("%s::%s----", __FILE__, __func__);printf(__VA_ARGS__);} while(false)
#ifndef hexdump
#define hexdump(data, len) do {for (uint32 i = 0; i < (uint32)len; i ++) { printf("%02x ", *(uint8 *)(data + i));} printf("\n");} while(0)
#endif
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
#endif

#include"Utils.h"
const uint32 DATA_BUF_LEN = 10240;
const uint32 DATA_BUF_REAL_LEN = 16384;
const uint32 TMP_BUF_LEN = 2048;
const uint32 TMP_BUF_REAL_LEN = 2064;
const uint8 AT_MAX_RETRY = 10;
const uint32 AT_BUFFER_MAX_LEN = 512;
const uint32 MIN_TIME_OUT = 60*ONE_SECOND;
const uint32 MAX_TIME_OUT = ONE_SECOND*60;
const char* AT_FLAG = "\r\n";
const char AT_COMMA = ',';
const char* ATCommandList[] =
{
   "AT\r\n",
   "ATE0\r\n",
   "AT+CGSN\r\n",
   "AT+CIPMODE=1\r\n",
   "AT+CIPSRIP=0\r\n",
   "AT+CIPHEAD=1\r\n",
   "AT+CIPSPRT=2\r\n",
   "AT+CREG?\r\n",
#ifdef CHINA_MOBILE
   "AT+CSTT=\"CMNET\"\r\n",
   "AT+CIPCSGP=1,\"CMNET\"\r\n",
#else
   "AT+CSTT=\"UNINET\"\r\n",
   "AT+CIPCSGP=1,\"UNINET\"\r\n",
#endif
   "AT+CGATT?\r\n",
   "AT+CIICR\r\n",
   "AT+CIFSR\r\n",
   "AT+CIPSTART=\"UDP\"",
   "AT+CIPSTATUS\r\n",
   "AT+CIPCLOSE\r\n",
   "AT+CIPSHUT\r\n",
   "AT+CPOWD=1\r\n",
   "AT+CBC\r\n",
   "AT+CLPORT=\"UDP\",%d\r\n",
   "AT+CSQ\r\n",
   "+CSQ:",

   "+++",
   "\r\nATO\r\n",

   "OK",
   "ERROR",

   ""//never delete this
};
enum
{
   AT_AT=0,
   AT_ATE0,
   AT_CGSN,
   AT_CIPMODE_1,
   AT_CIPSRIP_0,
   AT_CIPHEAD_1,
   AT_CIPSPRT_2,
   AT_CREG_QUERY,
   AT_CSTT_UNINET,
   AT_CIPCSGP_UNINET,
   AT_CGATT_QUERY,
   AT_CIICR,
   AT_CIFSR,
   AT_CIPSTART,
   AT_CIPSTATUS,
   AT_CIPCLOSE,
   AT_CIPSHUT,
   AT_CPOWD,
   AT_CBC,
   AT_CLPORT_SET,
   AT_CSQ_QUERY,
   AT_CSQ_ACK,

   AT_EXIT_TRANSFER_DATA_MODE,
   AT_ENTER_TRANSFER_DATA_MODE,

   AT_OK_ACK,
   AT_ERROR_ACK,

   AT_MAX
};

CGprs::CGprs():m_IsConnected(false), m_SrcPort(0), m_DestPort(0)
{
   memset(m_DestIP, 0, sizeof(m_DestIP));
   memset(m_IMEI, 0, sizeof(m_IMEI));
}

CGprs::CGprs(const string& CommName):CSerialComm(CommName)
                                     , m_IsConnected(false)
                                     , m_SrcPort(0)
                                     , m_DestPort(0)
{
   memset(m_DestIP, 0, sizeof(m_DestIP));
   memset(m_IMEI, 0, sizeof(m_IMEI));
}

CGprs::CGprs(const string& CommName, uint32 SrcPort, char* pDestIPStr, uint32 DestPort):CSerialComm(CommName)
                                                                                        , m_IsConnected(false)
                                                                                        , m_SrcPort(SrcPort)
                                                                                        , m_DestPort(DestPort)
{
   if(NULL == pDestIPStr)
   {
      return;
   }
   assert(IP_MAX_LEN > strlen(pDestIPStr));
   memset(m_DestIP, 0, sizeof(m_DestIP));
   strcpy(m_DestIP, pDestIPStr);
   memset(m_IMEI, 0, sizeof(m_IMEI));
}

void CGprs::SetIP(uint32 SrcPort, const char* pDestIPStr, uint32 DestPort)
{
   assert(NULL != pDestIPStr);
   if(NULL == pDestIPStr)
   {
      return;
   }
   assert(IP_MAX_LEN > strlen(pDestIPStr));
   m_SrcPort = SrcPort;
   memset(m_DestIP, 0, sizeof(m_DestIP));
   strcpy(m_DestIP, pDestIPStr);
   m_DestPort = DestPort;
}

bool CGprs::Connect()
{
   return Connect(m_DestIP, m_DestPort);
}

bool CGprs::Connect(const char * IP, const uint32 Port)
{
    if(NULL == IP)
    {
        return false;
    }

    if( false == IsOpen() )
    {
        return false;
    }
    strcpy(m_DestIP, IP);
    m_DestPort = Port;

    PowerOn();

    uint8 atbuf[LINE_REAL_LEN] = {0};
    uint32 len = LINE_LEN;
    int i = 0;
    for (i = 0; i < 3; i ++) {
        len = LINE_LEN;
        WaitAnyString(RX_TIMEOUT, (char *)atbuf, len);
        if (Command("AT\r\n") != COMM_OK) continue;
    }
    for (i = 0; i < 3; i ++) {
        if (Command("ATE0\r\n") != COMM_OK) continue;
    }

    bzero(atbuf, LINE_REAL_LEN);
    len = LINE_LEN;
    if (Command("AT+CGSN\r\n", RX_TIMEOUT, NULL, (char *)atbuf, len) != COMM_OK) return false;
    memcpy(m_IMEI, atbuf, strlen((char *)atbuf));
    DEBUG("m_IMEI=%s\n", (char*)m_IMEI);

    for (i = 0; i < 100; i ++) {
        len = LINE_LEN;
        bzero(atbuf, LINE_REAL_LEN);
        if (Command("AT+CREG?\r\n", RX_TIMEOUT << 1, "+CREG:", (char *)atbuf, len) != COMM_OK) continue;
        char * stat = strchr((char *)atbuf, ',');
        if (stat != NULL) {
            stat ++;
            if (stat[0] == '1' || stat[1] == '5') {
                break;
            }
        } else {
            continue;
        }
    }

    if (i == 100) {
        DEBUG("AT+CREG? ERROR\n");
        return false;
    }

    len = LINE_LEN;
    bzero(atbuf, LINE_REAL_LEN);
    if (Command("AT+CSQ\r\n", RX_TIMEOUT, "+CSQ:", (char *)atbuf, len) != COMM_OK) return false;
    char * sign = strchr((char *)atbuf, ':');
    sign ++;
    char * signend = strchr(sign, ',');
    * signend = 0;
    int signal = strtol(sign, NULL, 10);
    if (signal == 99) {
        DEBUG("No signal\n");
        return false;
    } else if (signal < 15) {
        DEBUG("signal(%d) is weak\n", signal);
        return false;
    }

    if (Command("AT+CGATT=1\r\n", NETREG_TIMEOUT) != COMM_OK) return false;
    if (Command("AT+CGATT?\r\n", NETREG_TIMEOUT, "+CGATT: 1") != COMM_OK)  return false;
    WaitString("OK", RX_TIMEOUT);

    if (Command("AT+CIPMODE=1\r\n") != COMM_OK) {
        DEBUG("Set TT mode error\n");
        return false;
    }

    if (Command("AT+CIPSRIP=1\r\n") != COMM_OK) return false;
    if (Command("AT+CIPHEAD=1\r\n") != COMM_OK) return false;
    if (Command("AT+CIPSPRT=2\r\n") != COMM_OK) return false;

    len = LINE_LEN;
    bzero(atbuf, len);
    sprintf((char *)atbuf, "AT+CLPORT=\"UDP\",%d\r\n", m_SrcPort);

    if (Command((char *)atbuf) != COMM_OK) return false;

    for (i = 0; i < 3; i ++) {
        if (Command("AT+CIPSHUT\r\n", "SHUT OK") != COMM_OK) continue;
        len = LINE_LEN;
        bzero(atbuf, len);
        sprintf((char *)atbuf, "AT+CIPSTART=\"UDP\",\"%s\",\"%d\"\r\n", m_DestIP, m_DestPort);
        if (Command((char *)atbuf, RX_TIMEOUT, NULL, (char *)atbuf, len) != COMM_OK) continue;
        if (strstr((char *)atbuf, "OK") != NULL || strstr((char *)atbuf, "ALREADY") != NULL) break;
    }

    if (i == 3) {
        DEBUG("Error\n");
        return false;
    }

    m_IsConnected = true;
    mode = WORK_MODE_TT;
    return true;
}

void CGprs::Disconnect()
{
   if( false == IsOpen() )
   {
      return;
   }
   m_IsConnected = false;
}

void CGprs::PowerOn()
{
   gpio_attr_t AttrOut;
   AttrOut.mode = PIO_MODE_OUT;
   AttrOut.resis = PIO_RESISTOR_DOWN;
   AttrOut.filter = PIO_FILTER_NOEFFECT;
   AttrOut.multer = PIO_MULDRIVER_NOEFFECT;
   SetPIOCfg(PB9, AttrOut);
   SetPIOCfg(PB10, AttrOut);

   PIOOutValue(PB10, 0);
   PIOOutValue(PB9, 1);
   sleep(1);

   gpio_attr_t AttrIn;
   AttrIn.mode = PIO_MODE_IN;
   AttrIn.resis = PIO_RESISTOR_DOWN;
   AttrIn.filter = PIO_FILTER_NOEFFECT;
   AttrIn.multer = PIO_MULDRIVER_NOEFFECT;
   SetPIOCfg(PB6, AttrIn);
   if(1 == PIOInValue(PB6) )
   {//GPRS module is in power on
      PIOOutValue(PB10, 1);
      sleep(1);
      PIOOutValue(PB10, 0);
      sleep(4);
   }

   PIOOutValue(PB10, 1);
   sleep(1);
   PIOOutValue(PB10, 0);
   sleep(4);
}

bool CGprs::GetIMEI(uint8* pIMEI, uint32& IMEILen)
{
   if(NULL == pIMEI || IMEILen < IMEI_LEN)
   {
      assert(0);
      return false;
   }
   memcpy(pIMEI, m_IMEI, IMEI_LEN);
   IMEILen = IMEI_LEN;
   return true;
}

bool CGprs::SwitchMode(enum GPRSWorkMode to) {
    if (this->mode == WORK_MODE_TT) {
        if (to == WORK_MODE_HTTP) {
            myusleep(500 * 1000);
            if (Command("+++", RX_TIMEOUT) != COMM_OK) goto switch_to_http_err0;
            if (Command("AT+CSQ\r\n", RX_TIMEOUT, "+CSQ:") != COMM_OK) goto switch_to_http_err1;
            if (Command("AT+CGATT?\r\n", RX_TIMEOUT, "+CGATT:") != COMM_OK) goto switch_to_http_err1;
            if (Command("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n") != COMM_OK) goto switch_to_http_err1;
#ifdef CHINA_MOBILE
            if (Command("AT+SAPBR=3,1,\"APN\",\"CMNET\"\r\n") != COMM_OK) goto switch_to_http_err1;
#else
            if (Command("AT+SAPBR=3,1,\"APN\",\"UNINET\"\r\n") != COMM_OK) goto switch_to_http_err1;
#endif
            if (Command("AT+SAPBR=1,1\r\n") != COMM_OK) goto switch_to_http_err1;
            if (Command("AT+HTTPINIT\r\n") != COMM_OK) goto switch_to_http_err1;

            this->mode = to;
            return true;
        }
    switch_to_http_err1:
        if (Command("ATO\r\n", RX_TIMEOUT, "CONNECT") != COMM_OK) {
            m_IsConnected = false;
        }
    switch_to_http_err0:
        return false;
    } else {
        if (to == WORK_MODE_TT) {
            if (Command("AT+HTTPTERM\r\n") != COMM_OK) return false;
            if (Command("ATO\r\n", RX_TIMEOUT, "CONNECT") != COMM_OK) return false;
            this->mode = to;
            return true;
        } else {
            return false;
        }
    }
}

ECommError CGprs::HttpGet(const char * url, uint8 * buf, size_t * size) {
    uint8 tmpbuf[TMP_BUF_REAL_LEN] = {0};
    uint32 len = TMP_BUF_LEN;
    if (false == m_IsConnected) {
        DEBUG("Disconnected\n");
        return COMM_FAIL;
    }
    if (this->mode != WORK_MODE_HTTP) {
        DEBUG("it's tt mode, switch to http mode\n");
        if (!SwitchMode(WORK_MODE_HTTP)) {
            DEBUG("Switch work mode fail\n");
            return COMM_FAIL;
        }
    } else {
        DEBUG("it's http mode\n");
    }

    if (Command("AT+HTTPPARA=\"CID\",1\r\n", RX_TIMEOUT) != COMM_OK) return COMM_FAIL;
    if (Command("AT+HTTPPARA=\"REDIR\",1\r\n", RX_TIMEOUT) != COMM_OK) return COMM_FAIL;
    bzero(tmpbuf, TMP_BUF_REAL_LEN);
    sprintf((char *)tmpbuf, "AT+HTTPPARA=\"URL\",\"%s\"\r\n", url);
    if (Command((char *)tmpbuf, RX_TIMEOUT) != COMM_OK) return COMM_FAIL;
    if (Command("AT+HTTPACTION=0\r\n", RX_TIMEOUT) != COMM_OK) return COMM_FAIL;

    bzero(tmpbuf, TMP_BUF_REAL_LEN);
    len = TMP_BUF_LEN;
    if (WaitString("+HTTPACTION:", HTTP_TIMEOUT, (char *)tmpbuf, len) != COMM_OK) return COMM_FAIL;

    char * s = strstr((char *)tmpbuf, ",");
    s ++;
    char * l = strstr(s, ",");
    * l = 0;
    l ++;
    if (strcmp(s, "200") != 0) {
        DEBUG("download failed, http status is %s\n", s);
        return COMM_FAIL;
    }
    len = strtol(l, NULL, 10);
    if ((size_t) len > * size) {
        DEBUG("download failed, data length %d > buffer length %d\n", len, * size);
        return COMM_FAIL;
    }
    * size = len;
    bzero(tmpbuf, TMP_BUF_REAL_LEN);
    if (Command("AT+HTTPREAD\r\n", RX_TIMEOUT, "+HTTPREAD:", (char *)tmpbuf, len) != COMM_OK) return COMM_FAIL;
    len = strtol((char *)(tmpbuf + 10), NULL, 10);
    if (ReadRawData(buf, len, RX_TIMEOUT) != COMM_OK) return COMM_FAIL;
    if (WaitString("OK", RX_TIMEOUT, (char *)tmpbuf, len) != COMM_OK) return COMM_FAIL;
    return COMM_OK;
}

ECommError CGprs::SendData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut)
{
    if (false == m_IsConnected) {
       DEBUG("Disconnected\n");
       return COMM_FAIL;
    }
    if (this->mode != WORK_MODE_TT) {
        if (!SwitchMode(WORK_MODE_TT)) {
            DEBUG("Switch work mode fail\n");
            return COMM_FAIL;
        }
    }
    if (false == m_IsConnected) {
       DEBUG("Disconnected\n");
       return COMM_FAIL;
    }
    return WriteBuf(pBuffer, BufferLen, TimeOut);
}

ECommError CGprs::SendData(uint8* pBuffer, uint32& BufferLen, int32 SendTimeOut, uint8* pAckBuffer, uint32& AckBufferLen, int32 RecTimeOut)
{
   DEBUG("SendData: ");
   hexdump(pBuffer, BufferLen);
   ECommError Ret = SendData(pBuffer, BufferLen, SendTimeOut);
   if(COMM_OK != Ret)
   {
      return Ret;
   }

   Ret = ReceiveData(pAckBuffer, AckBufferLen, RecTimeOut);
   DEBUG("ReceData: ");
   hexdump(pAckBuffer, AckBufferLen);

   return Ret;
}

ECommError CGprs::ReceiveData(uint8* pBuffer, uint32& BufferLen, int32 TimeOut)
{
    DEBUG("BufferLen=%d\n", BufferLen);
    if (false == m_IsConnected) {
       DEBUG("Disconnected\n");
       return COMM_FAIL;
    }
    if (this->mode != WORK_MODE_TT) {
        if (!SwitchMode(WORK_MODE_TT)) {
            DEBUG("Switch work mode fail\n");
            return COMM_FAIL;
        }
    }
    if (false == m_IsConnected) {
       DEBUG("Disconnected\n");
       return COMM_FAIL;
    }
    ECommError Ret = ReadBuf(pBuffer, BufferLen, TimeOut);
    DEBUG("BufferLen=%d, Ret=%d\n", BufferLen, Ret);
    return Ret;
}

ECommError CGprs::ReceiveData(uint8* pBuffer, uint32& BufferLen, const uint8* pBeginFlag, uint32 BeginFlagLen, uint32 BufferLenPos, int32 TimeOut)
{
    if(false == m_IsConnected) {
        DEBUG("NOT connect\n");
        return COMM_FAIL;
    }
    if (this->mode != WORK_MODE_TT) {
        if (!SwitchMode(WORK_MODE_TT)) {
            DEBUG("Switch work mode fail\n");
            return COMM_FAIL;
        }
    }
    if (false == m_IsConnected) {
        DEBUG("NOT connect\n");
        return COMM_FAIL;
    }

    if ((NULL==pBuffer) && (BufferLen<BufferLenPos + GPRS_PACKET_LEN_LEN)) {
        DEBUG("parameter error\n");
        return COMM_FAIL;
    }
    //flush read Buffer
    tcflush(m_hComm,TCIFLUSH);
    //find the begin flag
    uint8 ReadBuffer[MAX_GPRS_PACKET_LEN]={0};
    int32 BeginFlagIndex = -1;
    //int32 EndFlagIndex = -1;

    uint32 nTotalReadBytes = 0;
    uint8 i = 0;
    bool IsBufferLenFound = false;
    uint32 PacketLen = 0;
    for (; i < MAX_WRITEREAD_COUNT; i++) {
        uint32 nReadBytes = 0;
        if (IsBufferLenFound) {
            nReadBytes = PacketLen - nTotalReadBytes;
        } else {
            nReadBytes = BufferLenPos + GPRS_PACKET_LEN_LEN;
        }

        ECommError Ret = ReadBuf(ReadBuffer+nTotalReadBytes, nReadBytes, TimeOut);
        DEBUG("Retry %d, Result: %d, readbytes: %d, nTotalReadBytes: %d\n", i, Ret, nReadBytes, nTotalReadBytes);
        if (COMM_OK != Ret) {
            return Ret;
        }
        nTotalReadBytes += nReadBytes;

        if (IsBufferLenFound && (nTotalReadBytes >= PacketLen)) {
            break;
        }

        //find the BeginFlag
        if (pBeginFlag && (-1 == BeginFlagIndex) && (nTotalReadBytes >= BeginFlagLen)) {
            uint32 Index = 0;
            for (; Index < (nTotalReadBytes-BeginFlagLen); Index++) {
                if (0 == memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen)) {
                    break;
                }
            }
            if (memcmp(ReadBuffer+Index, pBeginFlag, BeginFlagLen)) {
                continue;
            }
            BeginFlagIndex = Index;
            DEBUG("Find BeginFlag  BeginFlagIndex=%d\n", BeginFlagIndex);
        }
        //get the packet len
        if ((-1 != BeginFlagIndex) && (false == IsBufferLenFound )) {
            memcpy( &PacketLen, ReadBuffer+BufferLenPos, sizeof(PacketLen) );
            PacketLen += 4;//the packet data should add CRC and header data
            if ((PacketLen<(BufferLenPos+GPRS_PACKET_LEN_LEN)) && (PacketLen>MAX_GPRS_PACKET_LEN)) {
                DEBUG("PacketLen=%u should be in [%u,%u]\n", PacketLen, (BufferLenPos+GPRS_PACKET_LEN_LEN), MAX_GPRS_PACKET_LEN);
                return COMM_FAIL;
            }
            IsBufferLenFound = true;
            i = 0;//retry from the beginning
        }
    }
    if ((i >= MAX_WRITEREAD_COUNT) || (PacketLen != nTotalReadBytes)) {
        DEBUG("PacketLen(%u):nTotalReadBytes(%d) Can't read valid data\n", PacketLen, nTotalReadBytes);
        return COMM_NODATA;
    }
    DEBUG("PacketLen=%u, nTotalReadBytes=%d\n", PacketLen, nTotalReadBytes);

    //copy buffer
    if (PacketLen > BufferLen) {
        DEBUG("Buffer NOT enough, %d bytes needed, memory size=%d\n", PacketLen, BufferLen);
        return COMM_MEMORY_NOT_ENOUGH;
    }
    memcpy(pBuffer, ReadBuffer+BeginFlagIndex, PacketLen);
    BufferLen = PacketLen;

    return COMM_OK;
}

bool CGprs::GetSignalIntesity(uint8& nSignalIntesity)
{
    if (false == m_IsConnected) {
        nSignalIntesity = 0;
        DEBUG("NOT connect\n");
        return true;
    }
    uint8 atbuf[LINE_REAL_LEN] = {0};
    uint32 len = LINE_LEN;
    bool goBackTT = false;

    if (mode == WORK_MODE_TT) {
        myusleep(500 * 1000);
        if (Command("+++", RX_TIMEOUT) != COMM_OK) {
            DEBUG("Cannot leave TT mode\n");
            return false;
        }
        goBackTT = true;
        myusleep(500 * 1000);
    }

    len = LINE_LEN;
    bzero(atbuf, LINE_REAL_LEN);
    if (Command("AT+CSQ\r\n", RX_TIMEOUT, "+CSQ:", (char *)atbuf, len) != COMM_OK) {
        if (goBackTT) {
            if (Command("ATO\r\n", RX_TIMEOUT, "CONNECT") != COMM_OK) {
                m_IsConnected = false;
            }
        }
        return false;
    }
    char * sign = strchr((char *)atbuf, ':');
    sign ++;
    char * signend = strchr(sign, ',');
    * signend = 0;
    int signal = strtol(sign, NULL, 10);
    nSignalIntesity = signal;
    DEBUG("singal: %s\n", sign);

    if (goBackTT) {
        if (Command("ATO\r\n", RX_TIMEOUT, "CONNECT") != COMM_OK) {
            m_IsConnected = false;
        }
        return false;
    }

    return true;
}

ECommError CGprs::Command(const char * cmd, int32 timeout, const char * reply, char * buf, uint32 & len) {
    uint32 l = (uint32)strlen(cmd);
    Delay(99 * 1000);   // to prevent transmission corruption
    DEBUG("Send %s ", cmd);
    if (COMM_OK != WriteBuf((uint8 *)cmd, l, timeout)) {
        return COMM_FAIL;
    }
    Delay(99 * 1000);   // to prevent transmission corruption
    if (reply != NULL && strlen(reply) > 0)
        return WaitString(reply, timeout, buf, len);
    else
        return WaitAnyString(timeout, buf, len);
}

ECommError CGprs::WaitString(const char * str, int32 timeout, char * buf, uint32 & len) {
    int32 retry = 0;
    uint32 idx = 0;
    uint32 one = 1;

    DEBUG("Wait for %s\n", str);
    while (retry < timeout && idx < len) {
        one = 1;
        if (COMM_OK != ReadBuf((uint8 *)(buf + idx), one, 1)) {
            retry ++;
            continue;
        }
        if (one == 0) {
            retry ++;
            Delay(1);
            continue;
        }

        if (buf[idx] == '\r') continue; // <cr>
        else if (buf[idx] == 0) {
            retry ++;
            Delay(1);
            continue;
        } else if (buf[idx] == '\n') { // line received
            if (idx != 0) {
                buf[idx] = 0;

                DEBUG("ATResponse: %d bytes, %s\n", idx, buf);

                if (strncmp(buf, str, strlen(str)) == 0) {
                    len = idx;
                    return COMM_OK;
                } else if (strstr(buf, "ERROR") != NULL) {
                    len = idx;
                    return COMM_FAIL;
                } else {
                    idx = 0;
                }
            }
            retry ++;
            Delay(1);
        } else {
            idx ++;
        }
    }
    DEBUG("Waiting for %s timeouted!\n", str);
    return COMM_FAIL;
}

// Receive string of any length not including zero
ECommError CGprs::WaitAnyString(int32 timeout, char * buf, uint32 & len) {
    int32 retry = 0;
    uint32 idx = 0;
    uint32 one = 1;

    DEBUG("Wait for any string\n");
    while (retry < timeout && idx < len) {
        one = 1;
        if (COMM_OK != ReadBuf((uint8 *)(buf + idx), one, 1)) {
            retry ++;
            continue;
        }
        if (one == 0) {
            retry ++;
            Delay(1);
            continue;
        }

        if (buf[idx] == '\r') continue; // <cr>
        else if (buf[idx] == 0) {
            retry ++;
            Delay(1);
            continue;
        }
        else if (buf[idx] == '\n') { // line received
            buf[idx] = 0;

            if (idx > 0) {
                DEBUG("ATResponse: %d bytes, %s\n", idx, buf);
                len = idx;
                return COMM_OK;
            }
            retry ++;
            Delay(1);
        } else {
            idx ++;
        }
    }
    DEBUG("Waiting for any string timeouted!\n");
    return COMM_FAIL;
}

ECommError CGprs::ReadRawData(uint8 * buf, uint32 len, int32 timeout) {
    int32 retry = 0;
    uint32 idx = 0;
    uint32 one = 1;
    while (idx < len && retry < timeout) {
        one = 1;
        if (COMM_OK != ReadBuf(buf + idx, one, 1)) {
            retry ++;
            continue;
        }
        idx ++;
    }
    if (idx == len) {
        DEBUG("ATResponse: %d bytes\n", len);
        for (uint32 i = 0; i < len; i ++) {
            DEBUG("%02X ", (unsigned int)(buf[i]));
            if (i % 8 == 7) {
                DEBUG(" ");
            }
            if (i % 16 == 15) {
                DEBUG("\n");
            }
        }
        DEBUG("\n");
        return COMM_OK;
    } else {
        return COMM_FAIL;
    }
}

void CGprs::Delay(uint32 ms) {
    myusleep(ms * 1000);
}
