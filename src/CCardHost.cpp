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

/*
// only for test
uint8 sample[] = {
    // query time
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x64, 0x1E, 0xD0
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x65, 0xDF, 0x10
    //0x55, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x58, 0x40
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x61, 0xDE, 0xD3
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x62, 0x9E, 0xD2
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x63, 0x5F, 0x12
    //0x55, 0x09, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x64, 0x1E, 0xD0

    // query user
    //0x55, 0x0B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x01, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x1F, 0x73

    // prepaid
    //0x55, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x09, 0x11, 0x15, 0x20, 0x10, 0x03, 0x15, 0x00, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x38, 0x39, 0x30, 0x30, 0x30, 0x33, 0x47, 0xAF, 0x46, 0x58, 0x17, 0x54
};
*/

void hexdump(const uint8* data, const uint32 len) {
    uint32 i = 0;
    for(; i < len; i++) {
#ifdef DEBUG_CARDHOST
        printf("%02x ", data[i]);
#endif
    }
#ifdef DEBUG_CARDHOST
        printf("\n");
#endif
}

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
        } //else ParseAndExecute(sample, sizeof(sample)); // only for test
    }
    return 0;
}

void CCardHost::ParseAndExecute(uint8 *cmd, uint16 length) {
    uint8 * src, * dst, * code, * data, * crc;
    uint16 len;
    uint16 ptr = 0, gcrc;
    PrintData(cmd, length);
    if (cmd[0] != 0x55) {
        DEBUG("Not come from card host\n");
        return;
    }
    if (cmd[1] == 0xFF) {
        len = cmd[2];
        len = len << 8;
        len |= cmd[3];
        ptr = 4;
    } else {
        len = cmd[1];
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
    if (((gcrc>>8) & 0xFF) != crc[0] || (gcrc & 0xFF) != crc[1]) {
        DEBUG("CRC error want %0x %0x, generate %0x %0x\n", crc[0], crc[1], (gcrc & 0xFF), ((gcrc >> 8) & 0xFF));
        return;
    }

    switch (*code) {
    case CMD_QUERY:
        DEBUG("Query user command\n");
        PrintData(cmd, length);
        HandleQueryUser(data, crc - data);
        break;
    case CMD_PREPAID:
        DEBUG("Prepaid command\n");
        PrintData(cmd, length);
        HandlePrepaid(data, crc - data);
        break;
    case CMD_GETTIME:
        DEBUG("Get time command\n");
        PrintData(cmd, length);
        HandleGetTime(data, crc - data);
        break;
    default:
        DEBUG("Unknow command\n");
        PrintData(cmd, length);
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
        if (memcmp(valveIter->ValveData.ValveTemperature.UserID, buf, USERID_LEN) == 0 && valveIter->IsActive) {
            if (len == USERID_LEN) {
                // just user id, no more data to send
                AckQueryUser(NULL, 0);
            } else {
                // send the command to valve
                CForwarderMonitor::GetInstance()->QueryUser(valveIter->ValveData.ValveTemperature.UserID, buf + USERID_LEN, len - USERID_LEN);
            }
            return;
        }
    }
    AckQueryUser(NULL, 0);
}


void CCardHost::HandlePrepaid(uint8 * buf, uint16 len) {
    vector<ValveElemT> valves;
    int retry = 3;
    do {
        if (CForwarderMonitor::GetInstance()->GetValveList(valves))
            break;
        retry --;
    } while (retry > 0);
    if (retry == 0) {
        AckPrepaid(NULL, 0);
        return;
    }

    for(vector<ValveElemT>::iterator valveIter = valves.begin(); valveIter != valves.end(); valveIter++) {
        if (memcmp(valveIter->ValveData.ValveTemperature.UserID, buf, USERID_LEN) == 0 && valveIter->IsActive) {
            // send the command to valve
            CForwarderMonitor::GetInstance()->Prepaid(valveIter->ValveData.ValveTemperature.UserID, buf + USERID_LEN, len - USERID_LEN);
            return;
        }
    }
    AckPrepaid(NULL, 0);
}

void CCardHost::HandleGetTime(uint8 * data, uint16 len) {
    if (len == 0) {
        AckTimeOrRemove(NULL, 0);
    } else {
        // todo:
    }
}

void CCardHost::AckQueryUser(uint8 * data, uint16 len) {
    uint32 ptr = 0;
    uint16 crc = 0, cmdlen = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);
    if (buf == NULL) {
        DEBUG("No enough memory to ack query user command\n");
        return;
    }
    buf[ptr] = 0xAA;
    // skip cmd length
    if (len < 255) ptr += 2; else ptr += 4;
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
    cmdlen = 1 + len + 2; // status + data + crc
    if (cmdlen < 255) {
        buf[1] = cmdlen & 0xFF;
    } else {
        buf[1] = 0xFF;
        buf[2] = (cmdlen >> 8) & 0xFF;
        buf[3] = cmdlen & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = crc & 0xFF; ptr ++;
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}

void CCardHost::AckPrepaid(uint8 * data, uint16 len) {
    uint32 ptr = 0;
    uint16 crc = 0, cmdlen = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);
    if (buf == NULL) {
        DEBUG("No enough memory to ack prepaid command\n");
        return;
    }
    buf[ptr] = 0xAA;
    // skip cmd length
    if (len < 255) ptr += 2; else ptr += 4;
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
        buf[ptr] = 0x82; ptr ++;
    } else {
        buf[ptr] = 0x02; ptr ++;
        for (uint16 i = 0; i < len; i ++) {
            buf[ptr + i] = data[i];
        }
        ptr += len;
    }
    // set cmd length
    cmdlen = 1 + len + 2; // status + data + crc
    if (cmdlen < 255) {
        buf[1] = cmdlen & 0xFF;
    } else {
        buf[1] = 0xFF;
        buf[2] = (cmdlen >> 8) & 0xFF;
        buf[3] = cmdlen & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = crc & 0xFF; ptr ++;
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}

void CCardHost::AckTimeOrRemove(uint8 * data, uint16 len) {
    uint32 ptr = 0, time = 0;
    uint16 crc = 0, cmdlen = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);

    if (buf == NULL) {
        DEBUG("No enough memory to ack prepaid command\n");
        return;
    }

    if (len == 0) {
        if (!GetLocalTimeStamp(time)) {
            DEBUG("Cannot get local time, abort!\n");
            return;
        }
    }
    buf[ptr] = 0xAA;
    // skip cmd length
    if (len < 255) ptr += 2; else ptr += 4;
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
        buf[ptr] = 0x05; ptr ++;
        buf[ptr] = time & 0xFF; ptr ++;
        buf[ptr] = (time >> 8) & 0xFF; ptr ++;
        buf[ptr] = (time >> 16) & 0xFF; ptr ++;
        buf[ptr] = (time >> 24) & 0xFF; ptr ++;
        len = 4;
    } else {
        buf[ptr] = 0x85; ptr ++;
        for (uint16 i = 0; i < len; i ++) {
            buf[ptr + i] = data[i];
        }
        ptr += len;
    }
    // set cmd length
    cmdlen = 1 + len + 2; // status + data + crc
    if (cmdlen < 255) {
        buf[1] = cmdlen & 0xFF;
    } else {
        buf[1] = 0xFF;
        buf[2] = (cmdlen >> 8) & 0xFF;
        buf[3] = cmdlen & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = crc & 0xFF; ptr ++;
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}
