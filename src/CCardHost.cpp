#include "CCardHost.h"
#include "Utils.h"
#include "CForwarderMonitor.h"

#include "libs_emsys_odm.h"

#ifdef DEBUG_CARDHOST
#define DEBUG(...) do {printf("%s::%s----", __FILE__, __func__);printf(__VA_ARGS__);} while(false)
#else
#define DEBUG(...)
#endif

#define PKTLEN 512

#define CMD_QUERY    0x01
#define CMD_PREPAID  0x02
#define CMD_GETTIME  0x05

CCardHost * CCardHost::instance = NULL;
CCardHost * CCardHost::GetInstance()
{
    if (instance == NULL) {
        instance = new CCardHost();
    }
    return instance;
}

CCardHost::CCardHost() {
    cmdbuf = cbuffer_create(10, 512);
}

CCardHost::~CCardHost() {
    cbuffer_free(cmdbuf);
}

uint32 CCardHost::Run() {
    fd_set rfds, wfds;
    struct timeval tv;
    int retval, r, readed = 0, w, wrote = 0;
    uint8 cmd[PKTLEN];
    uint8 * buf = NULL;
    uint16 rlen = 0, wlen = 0;
    while (true) {

        FD_ZERO(&rfds);
        FD_SET(com, &rfds);

        FD_ZERO(&wfds);
        if (cbuffer_read(cmdbuf) != NULL)
            FD_SET(com, &wfds);

        /* Wait up to five seconds. */
        tv.tv_sec = 5;
        tv.tv_usec = 0;

        retval = select(com + 1, &rfds, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = PKTLEN;
                    bzero(cmd, PKTLEN);
                }

                r = read(com, cmd + readed, rlen);

                if (r == -1) {
                    continue;
                }
                readed += r;

                if (rlen == PKTLEN && readed > 1 && cmd[1] != 0xFF) {
                    rlen = cmd[1] + 1; //include cmd header
                } else if (rlen == PKTLEN && readed > 3 && cmd[1] == 0xFF) {
                    rlen = cmd[2] << 8 + buf[3] + 1; //include cmd header
                }
                if (readed == rlen) {
                    readed = 0;
                    ParseAndExecute(cmd, rlen);
                }
            }
            if (FD_ISSET(com, &wfds)) {
                buf = (uint8 *) cbuffer_read(cmdbuf);
                if (buf == NULL) continue; // no acks to send
                if (wrote == 0) {
                    if (buf[1] == 0xFF) {
                        wlen = buf[2] << 8 + buf[3] + 1; //include cmd header
                    } else {
                        wlen = buf[1] + 1; //include cmd header
                    }
                }
                w = write(com, buf + wrote, wlen - wrote);
                if (w == -1) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    cbuffer_read_done(cmdbuf);
                    wrote = 0;
                }
            }
        }
    }
    return 0;
}

void CCardHost::ParseAndExecute(uint8 *cmd, uint16 length) {
    uint8 * src, * dst, * code, * data, * crc;
    uint16 len = length - 1; // exclude cmd header
    uint16 ptr = 0, gcrc;
    if (cmd[0] != 0x55) {
        DEBUG("Not come from card host\n");
        return;
    }
    if (cmd[1] == 0xFF) {
        ptr = 4;
    } else {
        ptr = 2;
    }
    src = cmd + ptr;
    dst = cmd + ptr + 6;
    ptr += 12;
    if (src[0] != 0x0 ||
        src[1] != 0x0 ||
        src[2] != 0x0 ||
        src[3] != 0x0 ||
        src[4] != 0x0 ||
        src[5] != 0x0) {
        DEBUG("Src addr incorrect, may not come from card host\n");
        return;
    }
    if (dst[0] != 0xff ||
        dst[1] != 0xff ||
        dst[2] != 0xff ||
        dst[3] != 0xff ||
        dst[4] != 0xff ||
        dst[5] != 0xff) {
        DEBUG("Dst addr incorrect, may not send to me\n");
        return;
    }
    code = cmd + ptr;
    ptr ++;
    data = cmd + ptr;
    ptr += len - 3;
    crc = cmd + ptr;
    gcrc = GenerateCRC(cmd, ptr);
    if (((gcrc>>8) & 0xFF) != crc[1] || (gcrc & 0xFF) != crc[0]) {
        DEBUG("CRC error\n");
        return;
    }

    switch (*code) {
    case CMD_QUERY:
        DEBUG("Query user command\n");
        HandleQueryUser(data, crc - data);
        break;
    case CMD_PREPAID:
        DEBUG("Prepaid command\n");
        break;
    case CMD_GETTIME:
        DEBUG("Get time command\n");
        HandleGetTime();
        break;
    }
}

void CCardHost::HandleQueryUser(uint8 * buf, uint16 len) {
    vector<ValveElemT> valves;
    int retry = 3;
    do {
        if (CForwarderMonitor::GetInstance()->GetValveList(valves))
            break;
        retry --;
    } while (retry > 0);
    if (retry == 0) {
        AckQueryUser(NULL, 0);
        return;
    }

    for(vector<ValveElemT>::iterator valveIter = valves.begin(); valveIter != valves.end(); valveIter++) {
        if (memcmp(valveIter->ValveData.ValveTemperature.UserID, buf, len) == 0 && valveIter->IsActive) {
            if (len == USERID_LEN) {
                // just user id, no more data to send
                AckQueryUser(NULL, 0);
            } else {
                // todo: send the command to valve
                CForwarderMonitor::GetInstance()->QueryUser(valveIter->ValveData.ValveTemperature.UserID, buf + USERID_LEN, len - USERID_LEN);
            }
            return;
        }
    }
    AckQueryUser(NULL, 0);
}

void CCardHost::HandleGetTime() {
    uint32 ptr = 0, time = 0;
    uint16 crc = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);
    if (buf) {
        if (!GetLocalTimeStamp(time)) {
            DEBUG("Cannot get local time, abort!\n");
            return;
        }
        buf[ptr] = 0xAA;
        ptr += 2; // skip cmd length
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x00; ptr ++;
        buf[ptr] = 0x05; ptr ++;
        *(uint32 *)(buf + ptr) = time; ptr += 4;
        buf[1] = ptr + 2; // set cmd length
        crc = GenerateCRC(buf, ptr);
        buf[ptr] = crc & 0xFF; ptr ++;
        buf[ptr] = (crc>>8) & 0xFF;
        cbuffer_write_done(cmdbuf);
    }
}

void CCardHost::AckQueryUser(uint8 * data, uint16 len) {
    uint32 ptr = 0;
    uint16 crc = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);
    if (buf == NULL) {
        DEBUG("No enough memory to ack query user command\n");
        return;
    }
    buf[ptr] = 0xAA;
    // skip cmd length
    if (len <= 238) ptr += 2; else ptr += 4;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    buf[ptr] = 0x00; ptr ++;
    if (len == 0) {
        buf[ptr] = 0x81; ptr ++;
    } else {
        buf[ptr] = 0x01; ptr ++;
        for (uint16 i = 0; i < len; i ++) {
            buf[ptr + i] = data[i];
        }
        ptr += len;
    }
    // set cmd length
    if (len <= 238) {
        buf[1] = ptr + 2;
    } else {
        buf[1] = 0xFF;
        buf[2] = ((ptr + 2) >> 8) & 0xFF;
        buf[3] = (ptr + 2) & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = crc & 0xFF; ptr ++;
    buf[ptr] = (crc>>8) & 0xFF;
    cbuffer_write_done(cmdbuf);
}
