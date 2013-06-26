#include "CCardHost.h"
#include "Utils.h"
#include "IValveMonitorFactory.h"

#ifdef DEBUG_CARDHOST
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

#define PKTLEN 512

#define CMD_QUERY    0x01
#define CMD_RECHARGE 0x02
#define CMD_GETTIME  0x05
#define CMD_INFO     0xF5

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

    // recharge
    //0x55, 0x5F, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x02, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x20, 0x09, 0x11, 0x15, 0x20, 0x10, 0x03, 0x15, 0x00, 0x31, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x32, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x31, 0x38, 0x39, 0x30, 0x30, 0x30, 0x33, 0x47, 0xAF, 0x46, 0x58, 0x17, 0x54
};
*/

CCardHost * CCardHost::instance = NULL;
CCardHost * CCardHost::GetInstance()
{
    if (instance == NULL) {
        instance = new CCardHost();
    }
    return instance;
}

CCardHost::CCardHost() {
    cmdbuf = cbuffer_create(10, PKTLEN);
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
    uint8 nullloop = 0;

    while (true) {

        FD_ZERO(&rfds);
        FD_SET(com, &rfds);

        FD_ZERO(&wfds);
        if (cbuffer_read(cmdbuf) != NULL) {
            FD_SET(com, &wfds);
            TX_ENABLE(gpio);
        } else {
            RX_ENABLE(gpio);
        }

        /* Wait up to five seconds. */
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        retval = select(com + 1, &rfds, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = 4;
                    bzero(cmd, PKTLEN);
                }

                r = read(com, cmd + readed, rlen - readed);

                if (r == -1 || r == 0) {
                    continue;
                }
                readed += r;

                if (rlen == 4 && readed > 1 && cmd[1] != 0xFF) {
                    rlen = 1 + 1 + 6 + 6 + cmd[1]; // header + len + src + dst + data
                } else if (rlen == 4 && readed > 3 && cmd[1] == 0xFF) {
                    rlen = 1 + 3 + 6 + 6 + ((uint16)cmd[2]) << 8 + cmd[3]; // header + len + src + dst + data
                }
                if (readed == rlen) {
                    readed = 0;
                    DEBUG("Read %d bytes: ", rlen);
                    hexdump(cmd, rlen);
                    ParseAndExecute(cmd, rlen);
                }
            }
            if (FD_ISSET(com, &wfds)) {
                buf = (uint8 *) cbuffer_read(cmdbuf);
                if (buf == NULL) continue; // no acks to send
                if (wrote == 0) {
                    if (buf[1] == 0xFF) {
                        wlen = 1 + 3 + 6 + 6 + ((uint16)buf[2]) << 8 + buf[3]; // header + len + src + dst + data
                    } else {
                        wlen = 1 + 1 + 6 + 6 + buf[1]; // header + len + src + dst + data
                    }
                }
                w = write(com, buf + wrote, wlen - wrote);
                if (w == -1 || w == 0) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    DEBUG("Write %d bytes:", wlen);
                    hexdump(buf, wlen);
                    cbuffer_read_done(cmdbuf);
                    wrote = 0;
                    sleep(1);
                }
            }
            nullloop = 0;
        } else {
            nullloop ++;
            if (nullloop % 16 == 15) {
                DEBUG("Nothing to read\n");
            }
        } //else ParseAndExecute(sample, sizeof(sample)); // only for test
    }
    return 0;
}

void CCardHost::ParseAndExecute(uint8 *cmd, uint16 length) {
    uint8 * src, * dst, * code, * data, * crc;
    uint16 len;
    uint16 ptr = 0, gcrc;
    hexdump(cmd, length);
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
        hexdump(cmd, length);
        HandleQueryUser(data, crc - data);
        break;
    case CMD_RECHARGE:
        DEBUG("Recharge command\n");
        hexdump(cmd, length);
        HandleRecharge(data, crc - data);
        break;
    case CMD_GETTIME:
        DEBUG("Get time command\n");
        hexdump(cmd, length);
        HandleGetTime(data, crc - data);
        break;
    case CMD_INFO:
        DEBUG("Query info command\n");
        hexdump(cmd, length);
        HandleInfo(data, crc - data);
        break;
    default:
        DEBUG("Unknow command\n");
        hexdump(cmd, length);
        break;
    }
}

void CCardHost::HandleQueryUser(uint8 * buf, uint16 len) {
    vector<user_t> users;
    int retry = 3;
    userid_t uid;
    bzero(uid.x, sizeof(userid_t));
    memcpy(uid.x, buf, sizeof(userid_t));
    do {
        if (IValveMonitorFactory::GetInstance()->GetUserList(users))
            break;
        retry --;
    } while (retry > 0);
    if (retry == 0) {
        DEBUG("Cannot get valve list\n");
        AckQueryUser(uid, NULL, 0);
        return;
    }
    DEBUG("Want USER ID: ");
    hexdump(buf, sizeof(userid_t));

    for (vector<user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
#ifdef DEBUG_CARDHOST
        uint8 * user = iter->uid.x;
#endif
        DEBUG("Iter USER ID: ");
        hexdump(user, sizeof(userid_t));
        if (memcmp(iter->uid.x, buf, USERID_LEN) == 0) {
            DEBUG("Found USER ID: ");
            hexdump(user, sizeof(userid_t));
            if (len == USERID_LEN) {
                // just user id, no more data to send
                AckQueryUser(uid, buf, 0);
            } else {
                // send the command to valve
                IValveMonitorFactory::GetInstance()->QueryUser(iter->uid, buf + USERID_LEN, len - USERID_LEN);
            }
            return;
        }
    }

    AckQueryUser(uid, NULL, 0);
}

void CCardHost::HandleRecharge(uint8 * buf, uint16 len) {
    vector<user_t> users;
    int retry = 3;
    userid_t uid;
    bzero(uid.x, sizeof(userid_t));
    memcpy(uid.x, buf, sizeof(userid_t));
    do {
        if (IValveMonitorFactory::GetInstance()->GetUserList(users))
            break;
        retry --;
    } while (retry > 0);
    if (retry == 0) {
        DEBUG("Cannot get valve list\n");
        AckRecharge(uid, NULL, 0);
        return;
    }
    DEBUG("Want USER ID: ");
    hexdump(buf, sizeof(userid_t));

    for (vector<user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
#ifdef DEBUG_CARDHOST
        uint8 * user = iter->uid.x;
#endif
        DEBUG("Iter USER ID: ");
        hexdump(user, sizeof(userid_t));
        if (memcmp(iter->uid.x, buf, USERID_LEN) == 0) {
            DEBUG("Found USER ID: ");
            hexdump(user, sizeof(userid_t));
            // send the command to valve
            IValveMonitorFactory::GetInstance()->Recharge(iter->uid, buf + USERID_LEN, len - USERID_LEN);
            return;
        }
    }
    AckRecharge(uid, NULL, 0);
}

void CCardHost::HandleGetTime(uint8 * data, uint16 len) {
    if (len == 0) {
        AckTimeOrRemove(NULL, 0);
    } else {
        AckTimeOrRemove(NULL, 0);
        DEBUG("Fault device: %02x %02x %02x %02x %02x %02x\n", data[0], data[1], data[2], data[3], data[4], data[5]);
    }
}

void CCardHost::HandleInfo(uint8 * data, uint16 len) {
    cardaddr_t addr;
    info.clear();
    for (uint16 i = 0, l = MIN((uint16)(data[0] * sizeof(cardaddr_t)), len - 1); i < l; i += sizeof(cardaddr_t)) {
        bzero(&addr.x, sizeof(cardaddr_t));
        memcpy(&addr.x, data + i + 1, sizeof(cardaddr_t));
        info.push_back(addr);
    }
    infoGotten = true;
}

void CCardHost::AckQueryUser(userid_t uid, uint8 * data, uint16 len) {
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
    if (data == NULL && len == 0) {
        buf[ptr] = 0x81; ptr ++;
        memcpy(buf + ptr, uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
    } else {
        buf[ptr] = 0x01; ptr ++;
        memcpy(buf + ptr, uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
        memcpy(buf + ptr, data, len); ptr += len;
    }
    // set cmd length
    cmdlen = 1 + sizeof(userid_t) + len + 2; // status + uid + data + crc
    if (cmdlen < 255) {
        buf[1] = cmdlen & 0xFF;
    } else {
        buf[1] = 0xFF;
        buf[2] = (cmdlen >> 8) & 0xFF;
        buf[3] = cmdlen & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    buf[ptr] = crc & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}

void CCardHost::AckRecharge(userid_t uid, uint8 * data, uint16 len) {
    DEBUG("Get %d bytes: ", len);
    hexdump(data, len);
    uint32 ptr = 0;
    uint16 crc = 0, cmdlen = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);
    if (buf == NULL) {
        DEBUG("No enough memory to ack recharge command\n");
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
        memcpy(buf + ptr, uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
    } else {
        buf[ptr] = 0x02; ptr ++;
        memcpy(buf + ptr, uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
        memcpy(buf + ptr, data, len); ptr += len;
    }
    // set cmd length
    cmdlen = 1 + sizeof(userid_t) + len + 2; // status + uid + data + crc
    if (cmdlen < 255) {
        buf[1] = cmdlen & 0xFF;
    } else {
        buf[1] = 0xFF;
        buf[2] = (cmdlen >> 8) & 0xFF;
        buf[3] = cmdlen & 0xFF;
    }
    crc = GenerateCRC(buf, ptr);
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    buf[ptr] = crc & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}

void CCardHost::AckTimeOrRemove(uint8 * data, uint16 len) {
    uint32 ptr = 0, time = 0;
    uint16 crc = 0, cmdlen = 0;
    uint8 * buf = (uint8 *)cbuffer_write(cmdbuf);

    if (buf == NULL) {
        DEBUG("No enough memory to ack recharge command\n");
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
        if (info.size() == 0) {
            buf[ptr] = 0xf5; ptr ++;
        } else {
            buf[ptr] = 0x05; ptr ++;
        }
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
    buf[ptr] = (crc>>8) & 0xFF; ptr ++;
    buf[ptr] = crc & 0xFF; ptr ++;
    cbuffer_write_done(cmdbuf);
    DEBUG("Response: ");
    hexdump(buf, ptr);
}

bool CCardHost::GetCardInfo(vector<cardaddr_t>& i) {
    if (!infoGotten) {
        return false;
    }
    i = info;
    return true;
}
