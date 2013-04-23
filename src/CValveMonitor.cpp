#include "CValveMonitor.h"
#include "Utils.h"
#include "logdb.h"
#include "libs_emsys_odm.h"
#include "CPortal.h"
#include "CCardHost.h"

#ifdef DEBUG_VALVE
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
#define PORT 0x0F
#define RAND 0xFB

#define VALVE_PACKET_FLAG 0xF1

CValveMonitor * CValveMonitor::instance = NULL;
CValveMonitor * CValveMonitor::GetInstance() {
    if (instance == NULL) {
        instance = new CValveMonitor();
    }
    return instance;
}

CValveMonitor::CValveMonitor():noonTimer(DAY_TYPE) {
    tx = cbuffer_create(10, PKTLEN);
}

CValveMonitor::~CValveMonitor() {
    cbuffer_free(tx);
}

bool CValveMonitor::Init(uint32 startTime, uint32 interval) {
    return punctualTimer.Start(interval * 60) && noonTimer.Start(startTime);
}

void CValveMonitor::LoadUsers() {
    LOGDB * db = dbopen((char *)"users", DB_RDONLY, sizeof(user_t));
    if (db != NULL) {
        user_t user;
        size_t r;
        do {
            r = dbseq(db, &user, R_NEXT);
            if (r > 0) {
                users[user.uid] = user;
            }
        } while (r > 0);
        dbclose(db);
    }
}

void CValveMonitor::SaveUsers() {
    LOGDB * db = dbopen((char *)"users", DB_NEW, sizeof(user_t));
    if (db != NULL) {
        for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
            dbput(db, &iter->second, sizeof(user_t));
        }
        dbclose(db);
        syncUsers = false;
    }
}

uint32 CValveMonitor::Run() {
    fd_set rfds, wfds;
    struct timeval tv;
    int retval, r, readed = 0, w, wrote = 0;
    uint8 ack[PKTLEN];
    uint8 * buf = NULL;
    uint16 rlen = 0, wlen = 0;
    time_t last = 0;

    LoadUsers();

    while (true) {

        FD_ZERO(&rfds);
        FD_SET(com, &rfds);
        FD_ZERO(&wfds);

        if (users.size() == 0 && time(NULL) - last > 600) {
            Broadcast();
            last = time(NULL);
        } else {
            if (punctualTimer.Done()) {
                GetPunctualData();
                GetTimeData();
                if (valveTemperatures.size() > 0) {
                    SendValveData();
                }
                if (syncUsers) {
                    SaveUsers();
                }
            }
            if (noonTimer.Done()) {
                GetRechargeData();
                GetConsumeData();
            }
        }

        if (cbuffer_read(tx) != NULL)
            FD_SET(com, &wfds);

        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(com + 1, &rfds, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = PKTLEN;
                    bzero(ack, PKTLEN);
                }

                r = read(com, ack + readed, rlen);

                if (r == -1) {
                    continue;
                }
                readed += r;

                if (rlen == PKTLEN && readed > 6 && ack[6] != 0xFF) {
                    rlen = 4 + 1 + 1 + 1 + ack[6] + 2; // mac + port + rand + len + data + crc
                } else if (rlen == PKTLEN && readed > 6 && ack[6] == 0xFF) {
                    rlen = 4 + 1 + 1 + 3 + ((uint16)ack[7]) << 8 + ack[8] + 2; // mac + port + rand + len + data + crc
                }
                if (readed == rlen) {
                    readed = 0;
                    DEBUG("read %d bytes: ", rlen);
                    hexdump(ack, rlen);
                    ParseAck(ack, rlen);
                }
            }
            if (FD_ISSET(com, &wfds)) {
                buf = (uint8 *) cbuffer_read(tx);
                if (buf == NULL) continue;
                if (wrote == 0) {
                    if (buf[6] == 0xFF) {
                        wlen = 4 + 1 + 1 + 3 + ((uint16)buf[7]) << 8 + buf[8] + 2; // mac + port + rand + len + data + crc
                    } else {
                        wlen = 4 + 1 + 1 + 1 + buf[6] + 2; // mac + port + rand + len + data + crc
                    }
                    uint16 crc = GenerateCRC(buf, wlen - 2);
                    buf[wlen - 2] = (crc >> 8) & 0xFF;
                    buf[wlen - 1] = crc & 0xFF;
                }
                w = write(com, buf + wrote, wlen - wrote);
                if (w == -1) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    DEBUG("Write %d bytes:", wlen);
                    hexdump(buf, wlen);
                    cbuffer_read_done(tx);
                    wrote = 0;
                    sleep(1);
                }
            }
        }
    }

    return 0;
}

void CValveMonitor::Broadcast() {
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = 0xFF; ptr ++;
        buf[ptr] = PORT; ptr ++;
        buf[ptr] = RAND; ptr ++;
        buf[ptr] = 0x01; ptr ++;
        buf[ptr] = VALVE_BROADCAST; ptr ++;
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

bool CValveMonitor::GetUserList(vector<user_t>& u) {
    if (users_lock.TryLock()) {
        for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
            u.push_back(iter->second);
        }
        users_lock.UnLock();
        return true;
    }
    return false;
}

void CValveMonitor::ParseAck(uint8 * ack, uint16 len) {
    uint16 gcrc = GenerateCRC(ack, len - 2);
    if ((gcrc & 0xFF) != ack[len - 1] && ((gcrc >> 8) & 0xFF) != ack[len - 2]) {
        DEBUG("CRC error want %0x %0x, generate %0x %0x\n", ack[len - 1], ack[len - 2], (gcrc & 0xFF), ((gcrc >> 8) & 0xFF));
        return;
    }
    uint32 mac = 0;
    memcpy(&mac, ack, sizeof(uint32));
    uint16 dlen;
    uint8 code, * data;
    if (ack[6] == 0xFF) {
        dlen = ((uint16)ack[7]) << 8 + ack[8] - 1; // exclude code
        code = ack[9];
        data = ack + 10;
    } else {
        dlen = ack[6] - 1; // exclude code
        code = ack[7];
        data = ack + 8;
    }
    switch (code) {
    case VALVE_GET_RECHARGE_DATA:
        ParseRechargeData(mac, data, dlen);
        break;
    case VALVE_GET_CONSUME_DATA:
        ParseConsumeData(mac, data, dlen);
        break;
    case VALVE_GET_TIME_DATA:
        ParseTimeData(mac, data, dlen);
        break;
    case VALVE_GET_PUNCTUAL_DATA:
        ParsePunctualData(mac, data, dlen);
        break;
    case VALVE_QUERY_USER:
        ParseQueryUser(mac, data, dlen);
        break;
    case VALVE_RECHARGE:
        ParseRecharge(mac, data, dlen);
        break;
    case VALVE_BROADCAST:
        user_t user;
        memcpy(user.uid.x, data, dlen);
        user.vmac = mac;
        users[user.uid] = user;
        DEBUG("Found user [%02x %02x %02x %02x %02x %02x %02x %02x] in valve [%02x %02x %02x %02x]\n", user.uid.x[0], user.uid.x[1], user.uid.x[2], user.uid.x[3], user.uid.x[4], user.uid.x[5], user.uid.x[6], user.uid.x[7], user.vmac & 0xFF, (user.vmac >> 8) & 0xFF, (user.vmac >> 16) & 0xFF, (user.vmac >> 24) & 0xFF);
        syncUsers = true;
        break;
    default:
        break;
    }
}

void CValveMonitor::GetPunctualData() {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            buf[ptr] = 0x01; ptr ++; // len
            buf[ptr] = VALVE_GET_PUNCTUAL_DATA; ptr ++;
            cbuffer_write_done(tx);
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParsePunctualData(uint32 vmac, uint8 * data, uint16 len) {

    uint16 ptr = 0;
    // transcation date
    uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint32 utc = DateTime2TimeStamp(year, month, day, hour, 0, 0);

    // temperatures
    uint16 avgTemp = (((uint16)data[ptr] << 8) & 0xFF00) | data[ptr]; ptr += 2;
    uint16 avgSetTemp = (((uint16)data[ptr] << 8) & 0xFF00) | data[ptr]; ptr += 2;

    // device open time
    uint8 openTime = data[ptr]; ptr ++;

    // device status
    uint16 status;
    memcpy(&status, data + ptr, 2); ptr += 2;

    // uid
    userid_t uid;
    memcpy(&uid.x, data + ptr, sizeof(userid_t)); ptr += sizeof(userid_t);

    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (memcmp(uid.x, iter->first.x, sizeof(userid_t)) == 0) {
            if (vmac != iter->second.vmac) {
                iter->second.vmac = vmac;
                syncUsers = true;
            }
        }
    }

    // device time
    year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    utc = DateTime2TimeStamp(year, month, day, hour, minute, second);

    if (records.count(vmac) == 0) {
        record_t rec;
        rec.recharge = data[ptr] - 1; ptr ++;
        rec.consume = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        rec.temperature = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        rec.status = data[ptr] - 1; ptr ++;
        records[vmac] = rec;
    } else {
        records[vmac].recharge = data[ptr] - 1; ptr ++;
        records[vmac].consume = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        records[vmac].temperature = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        records[vmac].status = data[ptr] - 1; ptr ++;
    }

    if (valveTemperatures.count(vmac) == 0) {
        valve_temperature_t tmp;
        tmp.mac = vmac;
        tmp.uid = uid;
        tmp.indoorTemp = avgTemp;
        tmp.setTemp = avgSetTemp;
        tmp.openTime = openTime;
        tmp.timestamp = utc;
        valveTemperatures[vmac] = tmp;
    } else {
        valveTemperatures[vmac].mac = vmac;
        valveTemperatures[vmac].uid = uid;
        valveTemperatures[vmac].indoorTemp = avgTemp;
        valveTemperatures[vmac].setTemp = avgSetTemp;
        valveTemperatures[vmac].openTime = openTime;
        valveTemperatures[vmac].timestamp = utc;
    }
}

void CValveMonitor::GetRechargeData() {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            buf[ptr] = 0x02; ptr ++; // len
            buf[ptr] = VALVE_GET_RECHARGE_DATA; ptr ++;
            buf[ptr] = records[iter->second.vmac].recharge; ptr ++;
            cbuffer_write_done(tx);
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseRechargeData(uint32 vmac, uint8 * data, uint16 len) {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    bzero(buf, PKTLEN);
    buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
    buf[ptr] = sizeof(userid_t) + len; ptr ++;
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            memcpy(buf + ptr, iter->first.x, sizeof(userid_t)); ptr += sizeof(userid_t);
            * (uint32 *)(buf + ptr) = vmac; ptr += sizeof(uint32);
            memcpy(buf + ptr, data, len); ptr += len;
            CPortal::GetInstance()->InsertForwarderChargeData(buf, ptr);
            return;
        }
    }
}

void CValveMonitor::GetConsumeData() {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            buf[ptr] = 0x03; ptr ++; // len
            buf[ptr] = VALVE_GET_CONSUME_DATA; ptr ++;
            buf[ptr] = records[iter->second.vmac].consume / 256; ptr ++;
            buf[ptr] = records[iter->second.vmac].consume % 256; ptr ++;
            cbuffer_write_done(tx);
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseConsumeData(uint32 vmac, uint8 * data, uint16 len) {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    bzero(buf, PKTLEN);
    buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
    buf[ptr] = sizeof(userid_t) + len; ptr ++;
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            * (uint32 *)(buf + ptr) = vmac; ptr += sizeof(uint32);
            memcpy(buf + ptr, iter->first.x, sizeof(userid_t)); ptr += sizeof(userid_t);
            memcpy(buf + ptr, data + 4, 4); ptr += 24; // used time unit

            uint8 tmp[7] = {0};
            tmp[6] = data[0];//yy
            tmp[5] = data[1];//yy
            tmp[4] = data[2];//mm
            tmp[3] = data[3];//dd
            uint32 timestamp = DateTime2TimeStamp(tmp, 7);
            memcpy(buf + ptr, &timestamp, sizeof(uint32)); ptr += sizeof(uint32);

            CPortal::GetInstance()->InsertForwarderConsumeData(buf, ptr);
            return;
        }
    }
}

void CValveMonitor::QueryUser(userid_t uid, uint8 * data, uint16 len) {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (memcmp(iter->first.x, uid.x, sizeof(userid_t)) == 0) {
            txlock.Lock();
            uint8 * buf = (uint8 *)cbuffer_write(tx);
            if (buf == NULL) {
                DEBUG("No enough memory for quering user\n");
                return;
            }
            bzero(buf, PKTLEN);
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            if ((1 + len) < 0xFF) {
                buf[ptr] = (1 + len) & 0xFF; ptr ++;// command + data
                buf[ptr] = VALVE_QUERY_USER; ptr ++;
                memcpy(buf + ptr, data, len);
            } else {
                buf[ptr] = 0xFF; ptr ++;
                buf[ptr] = ((1 + len) >> 8) & 0xFF; ptr ++; // high byte for 'command + data'
                buf[ptr] = (1 + len) & 0xFF; ptr ++; // low byte for 'command + data'
                buf[ptr] = VALVE_QUERY_USER; ptr ++;
                memcpy(buf + ptr, data, len);
            }
            cbuffer_write_done(tx);
            txlock.UnLock();
            return;
        }
    }
}

void CValveMonitor::ParseQueryUser(uint32 vmac, uint8 * data, uint16 len) {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            CCardHost::GetInstance()->AckQueryUser(iter->first, data, len);
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

void CValveMonitor::Recharge(userid_t uid, uint8 * data, uint16 len) {

    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (memcmp(iter->first.x, uid.x, sizeof(userid_t)) == 0) {
            txlock.Lock();
            uint8 * buf = (uint8 *)cbuffer_write(tx);
            if (buf == NULL) {
                DEBUG("No enough memory for recharge\n");
                return;
            }
            bzero(buf, PKTLEN);
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            if ((1 + len) < 0xFF) {
                buf[ptr] = (1 + len) & 0xFF; ptr ++;// command + data
                buf[ptr] = VALVE_RECHARGE; ptr ++;
                memcpy(buf + ptr, data, len);
            } else {
                buf[ptr] = 0xFF; ptr ++;
                buf[ptr] = ((1 + len) >> 8) & 0xFF; ptr ++; // high byte for 'command + data'
                buf[ptr] = (1 + len) & 0xFF; ptr ++; // low byte for 'command + data'
                buf[ptr] = VALVE_RECHARGE; ptr ++;
                memcpy(buf + ptr, data, len);
            }
            cbuffer_write_done(tx);
            txlock.UnLock();
            return;
        }
    }
}

void CValveMonitor::ParseRecharge(uint32 vmac, uint8 * data, uint16 len) {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            CCardHost::GetInstance()->AckRecharge(iter->first, data, len);
            return;
        }
    }
}

uint16 CValveMonitor::ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len) {
    uint16 okay = 0;

    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *)cbuffer_write(tx);
        uint16 ptr = 0;
        if (buf == NULL) {
            DEBUG("No enough memory for config valve\n");
            continue;
        }

        bzero(buf, PKTLEN);
        memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
        buf[ptr] = PORT; ptr ++;
        buf[ptr] = RAND; ptr ++;
        if ((1 + len) < 0xFF) {
            buf[ptr] = (1 + len) & 0xFF; ptr ++;// command + data
            buf[ptr] = cmd; ptr ++;
            memcpy(buf + ptr, data, len);
        } else {
            buf[ptr] = 0xFF; ptr ++;
            buf[ptr] = ((1 + len) >> 8) & 0xFF; ptr ++; // high byte for 'command + data'
            buf[ptr] = (1 + len) & 0xFF; ptr ++; // low byte for 'command + data'
            buf[ptr] = cmd; ptr ++;
            memcpy(buf + ptr, data, len);
        }
        cbuffer_write_done(tx);
        okay ++;
        txlock.UnLock();
    }
    return okay;
}

Status CValveMonitor::GetStatus() {
    if (false == IsStarted()) {
        DEBUG("STATUS_ERROR\n");
        return STATUS_ERROR;
    } else if (users.size() == 0) {
        DEBUG("STATUS_OFFLINE\n");
        return STATUS_OFFLINE;
    } else {
        DEBUG("STATUS_OK\n");
        return STATUS_OK;
    }
}

void CValveMonitor::GetTimeData() {
    for (map<userid_t, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            buf[ptr] = 0x01; ptr ++; // len
            buf[ptr] = VALVE_GET_TIME_DATA; ptr ++;
            cbuffer_write_done(tx);
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseTimeData(uint32 vmac, uint8 * data, uint16 len) {
    uint16 ptr = 0;
    if (valveTemperatures.count(vmac) == 0) {
        valve_temperature_t tmp;
        memcpy(&tmp.totalTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
        memcpy(&tmp.remainingTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
        memcpy(&tmp.runningTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
        valveTemperatures[vmac] = tmp;
    } else {
        memcpy(&valveTemperatures[vmac].totalTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
        memcpy(&valveTemperatures[vmac].remainingTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
        memcpy(&valveTemperatures[vmac].runningTime, data + ptr, sizeof(uint64)); ptr += sizeof(uint64);
    }
}

void CValveMonitor::SendValveData() {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    for (map<uint32, valve_temperature_t>::iterator iter = valveTemperatures.begin(); iter != valveTemperatures.end(); iter ++) {
        bzero(buf, PKTLEN);
        ptr = 0;
        buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
        buf[ptr] = 1 + 1 + sizeof(valve_temperature_t); ptr ++;
        memcpy(buf + ptr, &iter->second, sizeof(valve_temperature_t)); ptr += sizeof(valve_temperature_t);
        CPortal::GetInstance()->InsertForwarderData(buf, ptr);
    }
}
