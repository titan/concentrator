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

extern gpio_attr_t gpioattr;

void vuserseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen) {
    map<uint32, user_t> * users = (map<uint32, user_t> *) params;
    uint32 vmac = (uint32) key;
    user_t * user = (user_t *) data;
    (* users)[vmac] = * user;
}

void vrecordseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen) {
    map<uint32, record_t> * records = (map<uint32, record_t> *) params;
    uint32 * vmac = (uint32 *) key;
    record_t * record = (record_t *) data;
    (* records)[* vmac] = * record;
}

CValveMonitor * CValveMonitor::instance = NULL;
CValveMonitor * CValveMonitor::GetInstance() {
    if (instance == NULL) {
        instance = new CValveMonitor();
    }
    return instance;
}

CValveMonitor::CValveMonitor():noonTimer(DAY_TYPE), sid(0), counter(0) {
    tx = cbuffer_create(12, PKTLEN);
    if (access("data/", F_OK) == -1) {
        mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
}

CValveMonitor::~CValveMonitor() {
    cbuffer_free(tx);
}

bool CValveMonitor::Init(uint32 startTime, uint32 interval) {
    return punctualTimer.Start(interval * 60) && noonTimer.Start(startTime);
}

void CValveMonitor::LoadUsers() {
    LOGDB * db = dbopen((char *)"data/users", DB_RDONLY, genuserkey);
    if (db != NULL) {
        dbseq(db, vuserseq, &lastUsers, sizeof(lastUsers));
        dbclose(db);
    }
}

void CValveMonitor::SaveUsers() {
    LOGDB * db = dbopen((char *)"data/users", DB_NEW, genuserkey);
    if (db != NULL) {
        for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
            dbput(db, (void *)&iter->first, sizeof(uint32), &iter->second, sizeof(user_t));
        }
        dbclose(db);
        syncUsers = false;
        lastUsers.clear();
        for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
            lastUsers[iter->first] = iter->second;
        }
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
    uint32 rc = 0, wc = 0; // read and write counter
    uint8 retry = 0; // read retry times
    uint16 wi = 0; // write interval, ? seconds

    LoadUsers();
    LoadRecords();

    BroadcastClearSID();
    sleep(1);
    BroadcastClearSID();
    sleep(1);
    BroadcastClearSID();
    sleep(1);

    srandom(time(NULL));
    sid = random();

    Broadcast(++counter);
    Broadcast(++counter);
    Broadcast(++counter);
    BroadcastQuery();

    while (true) {

        FD_ZERO(&rfds);
        FD_SET(com, &rfds);
        FD_ZERO(&wfds);

        if (punctualTimer.Done()) {
            SyncValveTime();
            GetPunctualData();
            GetTimeData();
            if (valveDataType == VALVE_DATA_TYPE_HEAT) {
                GetHeatData();
            }
            if (valveTemperatures.size() > 0 || valveHeats.size() > 0) {
                SendValveData();
            }
            if (users.size() != valveCount) {
                DEBUG("Defined valve count: %d, found valve count: %d\n", valveCount, users.size());
                SendErrorValves();
            }
            if (syncUsers) {
                SaveUsers();
            }
            if (syncRecords) {
                SaveRecords();
            }
            BroadcastQuery();
        }
        if (noonTimer.Done()) {
            GetRechargeData();
            GetConsumeData();
        }

        if (cbuffer_read(tx) != NULL && rc == wc && time(NULL) - last > wi) {
            time_t t = time(NULL);
            DEBUG("%d - %d = %d, greater than %d?\n", t, last, t - last, wi);
            FD_SET(com, &wfds);
            TX_ENABLE(gpio);
        } else {
            RX_ENABLE(gpio);
        }

        tv.tv_sec = 1;
        tv.tv_usec = 0;
        retval = select(com + 1, &rfds, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = PKTLEN;
                    bzero(ack, PKTLEN);
                }

                r = read(com, ack + readed, rlen);

                if (r == -1 || r == 0) {
                    if (retry > 3) {
                        if (rc != wc)
                            DEBUG("Read timeout, retry: %d, rc: %d, wc: %d\n", retry, rc, wc);
                        retry = 0;
                        rc = wc;
                    } else {
                        retry ++;
                    }
                    continue;
                }
                DEBUG("read %d bytes: ", r);
                hexdump(ack + readed, r);
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
                    rc ++;
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
                if (w == -1 || w == 0) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    uint8 code;
                    if (buf[6] == 0xFF) {
                        code = buf[9];
                    } else {
                        code = buf[7];
                    }
                    if (code == 0xFF) {
                        wi = 60 * 2;
                    } else {
                        wi = 0;
                    }
                    last = time(NULL);
                    DEBUG("Write %d bytes:", wlen);
                    hexdump(buf, wlen);
                    cbuffer_read_done(tx);
                    wrote = 0;
                    sleep(1);
                    wc ++;
                }
            }
        } else {
            if (retry > 3) {
                if (rc != wc)
                    DEBUG("Read timeout, retry: %d, rc: %d, wc: %d\n", retry, rc, wc);
                retry = 0;
                rc = wc;
            } else {
                retry ++;
            }
        }
    }

    return 0;
}

void CValveMonitor::BroadcastClearSID() {
    uint8 buf[PKTLEN];
    uint8 ptr = 0;
    bzero(buf, PKTLEN);
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = 0xFF; ptr ++;
    buf[ptr] = PORT; ptr ++;
    buf[ptr] = RAND; ptr ++;
    buf[ptr] = 0x01; ptr ++;
    buf[ptr] = VALVE_BROADCAST_CLEAR_SID; ptr ++;
    SendCommand(buf, ptr);
}

void CValveMonitor::Broadcast(uint8 counter) {
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
        buf[ptr] = 0x06; ptr ++;
        buf[ptr] = VALVE_BROADCAST; ptr ++;
        buf[ptr] = counter; ptr ++;
        memcpy(buf + ptr, &sid, sizeof(uint32)); ptr += sizeof(uint32);
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::BroadcastQuery() {
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
        buf[ptr] = 0x06; ptr ++;
        buf[ptr] = VALVE_BROADCAST; ptr ++;
        buf[ptr] = 0xCC; ptr ++;
        memcpy(buf + ptr, &sid, sizeof(uint32)); ptr += sizeof(uint32);
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::ParseBroadcast(uint32 vmac, uint8 * data, uint16 len) {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    if (len == 8) {
        user_t user;
        memcpy(user.uid.x, data, sizeof(user_t));
        user.vmac = vmac;
        users[vmac] = user;
        DEBUG("Found user [%02x %02x %02x %02x %02x %02x %02x %02x] in valve [%02x %02x %02x %02x]\n", user.uid.x[0], user.uid.x[1], user.uid.x[2], user.uid.x[3], user.uid.x[4], user.uid.x[5], user.uid.x[6], user.uid.x[7], user.vmac & 0xFF, (user.vmac >> 8) & 0xFF, (user.vmac >> 16) & 0xFF, (user.vmac >> 24) & 0xFF);
        syncUsers = true;
        bzero(buf,PKTLEN);
        memcpy(buf, &vmac, sizeof(uint32)); ptr += sizeof(uint32);
        buf[ptr] = PORT; ptr ++;
        buf[ptr] = RAND; ptr ++;
        buf[ptr] = 0x06; ptr ++; // len
        buf[ptr] = VALVE_BROADCAST; ptr ++;
        buf[ptr] = 0xAC; ptr ++;
        memcpy(buf + ptr, &sid, sizeof(uint32)); ptr += sizeof(uint32);
        SendCommand(buf, ptr); // send only, on reponse
    } else if (len == 1) {
        Broadcast(counter ++);
        Broadcast(counter ++);
    }
}

bool CValveMonitor::GetUserList(vector<user_t>& u) {
    if (users_lock.TryLock()) {
        for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
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
    case VALVE_GET_TIME:
        ParseValveTime(mac, data, dlen);
        break;
    case VALVE_GET_RECHARGE_DATA:
        ParseRechargeData(mac, data, dlen);
        break;
    case VALVE_GET_CONSUME_DATA:
        ParseConsumeData(mac, data, dlen);
        break;
    case VALVE_GET_TIME_DATA:
        ParseTimeData(mac, data, dlen);
        break;
    case VALVE_GET_HEAT_DATA:
        ParseHeatData(mac, data, dlen);
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
        ParseBroadcast(mac, data, dlen);
        break;
    default:
        break;
    }
}

void CValveMonitor::GetPunctualData() {
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
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

    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (vmac == iter->second.vmac) {
            if (memcmp(uid.x, iter->second.uid.x, sizeof(userid_t)) != 0) {
                /*
                for (map<uint32, user_t>::iterator j = users.begin(); j != users.end(); j ++) {
                    if ((memcmp(j->second.uid.x, iter->second.uid.x, sizeof(userid_t)) == 0)) {
                        users.erase(j->first);
                        break;
                    }
                }
                memcpy(iter->second.uid.x, uid.x, sizeof(userid_t));
                syncUsers = true;
                */
            }
            break;
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
        rec.sentRecharge = 0;
        rec.consume = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        rec.sentConsume = 0;
        rec.temperature = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        rec.sentTemperature = 0;
        rec.status = data[ptr] - 1; ptr ++;
        rec.sentStatus = 0;
        records[vmac] = rec;
        syncRecords = true;
    } else {
        if (records[vmac].recharge != data[ptr] - 1) {
            syncRecords = true;
        }
        records[vmac].recharge = data[ptr] - 1; ptr ++;
        if (records[vmac].consume != data[ptr] * 256 + data[ptr + 1] - 1) {
            syncRecords = true;
        }
        records[vmac].consume = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        records[vmac].temperature = data[ptr] * 256 + data[ptr + 1] - 1; ptr += 2;
        records[vmac].status = data[ptr] - 1; ptr ++;
    }

    if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
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
}

void CValveMonitor::GetRechargeData() {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    const uint8 payload = 10;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        record_t * rec = &records[iter->second.vmac];
        while (rec->sentRecharge != rec->recharge & 0x7F) {
            uint8 recharge = rec->recharge;
            if (rec->recharge > 0x7F) recharge = rec->recharge & 0x7F;
            if (recharge < rec->sentRecharge) recharge += 20;
            uint8 delta = recharge - rec->sentRecharge;
            for (uint8 i = 0, j = delta / payload; i < j; i ++) {
                ptr = 0;
                memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
                buf[ptr] = PORT; ptr ++;
                buf[ptr] = RAND; ptr ++;
                buf[ptr] = 0x03; ptr ++; // len
                buf[ptr] = VALVE_GET_RECHARGE_DATA; ptr ++;
                buf[ptr] = rec->sentRecharge + i * payload + 1; ptr ++; // offset
                buf[ptr] = payload; ptr ++; // limit
                if (SendCommand(buf, ptr)) {
                    if (WaitCmdAck(buf, &ptr)) {
                        rec->sentRecharge += payload;
                        rec->sentRecharge %= 20;
                        ParseAck(buf, ptr);
                    } else {
                        DEBUG("Wait recharge cmd ack from %d to %d failed\n", rec->sentRecharge + i * payload + 1, rec->sentRecharge + i * payload + 1 + payload);
                        goto NEXT;
                    }
                } else {
                    DEBUG("Send cmd to get recharge data from %d to %d failed\n", rec->sentRecharge + i * payload + 1, rec->sentRecharge + i * payload + 1 + payload);
                    goto NEXT;
                }
            }
            if (delta % payload != 0) {
                ptr = 0;
                memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
                buf[ptr] = PORT; ptr ++;
                buf[ptr] = RAND; ptr ++;
                buf[ptr] = 0x03; ptr ++; // len
                buf[ptr] = VALVE_GET_RECHARGE_DATA; ptr ++;
                buf[ptr] = rec->sentRecharge + (delta / payload) * payload + 1; ptr ++; // offset
                buf[ptr] = delta % payload; ptr ++; // limit

                if (SendCommand(buf, ptr)) {
                    if (WaitCmdAck(buf, &ptr)) {
                        rec->sentRecharge += delta % payload;
                        rec->sentRecharge %= 20;
                        ParseAck(buf, ptr);
                    } else {
                        DEBUG("Wait recharge cmd ack from %d to %d failed\n", rec->sentRecharge + (delta / payload) * payload + 1, rec->sentRecharge + (delta / payload) * payload + 1 + delta % payload);
                        goto NEXT;
                    }
                } else {
                    DEBUG("Send cmd to get recharge data from %d to %d failed\n", rec->sentRecharge + (delta / payload) * payload + 1, rec->sentRecharge + (delta / payload) * payload + 1 + delta % payload);
                    goto NEXT;
                }
            }
            syncRecords = true;
        }
    NEXT:
        continue; // just make compiler happy
    }
}

void CValveMonitor::ParseRechargeData(uint32 vmac, uint8 * data, uint16 len) {
    uint8 buf[PKTLEN];
    const uint8 rsize = 42;
    uint16 ptr = 0;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            for (uint8 i = 0, count = len / rsize; i < count; i ++, ptr = 0) {
                bzero(buf, PKTLEN);
                buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
                buf[ptr] = sizeof(userid_t) + rsize; ptr ++;
                memcpy(buf + ptr, iter->second.uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
                * (uint32 *)(buf + ptr) = vmac; ptr += sizeof(uint32);
                memcpy(buf + ptr, data + i * rsize, rsize); ptr += rsize;
                CPortal::GetInstance()->InsertChargeData(buf, ptr);
            }
            return;
        }
    }
}

void CValveMonitor::GetConsumeData() {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    const uint8 payload = 32;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        record_t * rec = &records[iter->second.vmac];
        while (rec->sentConsume != rec->consume & 0x7FFF) {
            uint16 consume = rec->consume;
            if (rec->consume > 0x7FFF) consume = rec->consume & 0x7FFF;
            if (consume < rec->sentConsume) consume += 2400;
            uint8 delta = consume - rec->sentRecharge;
            for (uint8 i = 0, j = delta / payload; i < j; i ++) {
                ptr = 0;
                memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
                buf[ptr] = PORT; ptr ++;
                buf[ptr] = RAND; ptr ++;
                buf[ptr] = 0x04; ptr ++; // len
                buf[ptr] = VALVE_GET_CONSUME_DATA; ptr ++;
                buf[ptr] = (rec->sentConsume + i * payload + 1) / 256; ptr ++;
                buf[ptr] = (rec->sentConsume + i * payload + 1) % 256; ptr ++;
                buf[ptr] = payload; ptr ++;
                if (SendCommand(buf, ptr)) {
                    if (WaitCmdAck(buf, &ptr)) {
                        rec->sentConsume += payload;
                        rec->sentConsume %= 2400;
                        ParseAck(buf, ptr);
                    } else {
                        DEBUG("Wait consume cmd ack from %d to %d failed\n", rec->sentConsume + i * payload + 1, rec->sentConsume + i * payload + 1 + payload);
                        goto NEXT;
                    }
                } else {
                    DEBUG("Send cmd to get consume data from %d to %d failed\n", rec->sentConsume + i * payload + 1, rec->sentConsume + i * payload + 1 + payload);
                    goto NEXT;
                }
            }
            if (delta % payload != 0) {
                ptr = 0;
                memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
                buf[ptr] = PORT; ptr ++;
                buf[ptr] = RAND; ptr ++;
                buf[ptr] = 0x04; ptr ++; // len
                buf[ptr] = VALVE_GET_CONSUME_DATA; ptr ++;
                buf[ptr] = (rec->sentConsume + (delta / payload) * payload + 1) / 256; ptr ++;
                buf[ptr] = (rec->sentConsume + (delta / payload) * payload + 1) % 256; ptr ++;
                buf[ptr] = delta % payload; ptr ++;
                if (SendCommand(buf, ptr)) {
                    if (WaitCmdAck(buf, &ptr)) {
                        rec->sentConsume += delta % payload;
                        rec->sentConsume %= 2400;
                        ParseAck(buf, ptr);
                    } else {
                        DEBUG("Wait consume cmd ack from %d to %d failed\n", rec->sentConsume + (delta / payload) * payload + 1, rec->sentConsume + (delta / payload) * payload + 1 + delta % payload);
                        goto NEXT;
                    }
                } else {
                    DEBUG("Send cmd to get consume data from %d to %d failed\n", rec->sentConsume + (delta / payload) * payload + 1, rec->sentConsume + (delta / payload) * payload + 1 + delta % payload);
                    goto NEXT;
                }
            }
            syncRecords = true;
        }
    NEXT:
        continue; // just make compiler happy
    }
}

void CValveMonitor::ParseConsumeData(uint32 vmac, uint8 * data, uint16 len) {
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    const uint8 rsize = 8;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            for (uint8 i = 0, count = (len - 8) / rsize; i < count; i ++, ptr = 0) {
                bzero(buf, PKTLEN);
                buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
                buf[ptr] = sizeof(userid_t) + rsize; ptr ++;
                * (uint32 *)(buf + ptr) = vmac; ptr += sizeof(uint32);
                memcpy(buf + ptr, iter->second.uid.x, sizeof(userid_t)); ptr += sizeof(userid_t);
                memcpy(buf + ptr, data + i * rsize + 4, 4); ptr += 24; // used time unit and skip unset fields

                uint8 tmp[7] = {0};
                tmp[6] = data[i * rsize + 0];//yy
                tmp[5] = data[i * rsize + 1];//yy
                tmp[4] = data[i * rsize + 2];//mm
                tmp[3] = data[i * rsize + 3];//dd
                uint32 timestamp = DateTime2TimeStamp(tmp, 7);
                memcpy(buf + ptr, &timestamp, sizeof(uint32)); ptr += sizeof(uint32);

                CPortal::GetInstance()->InsertConsumeData(buf, ptr);
            }
            return;
        }
    }
}

void CValveMonitor::QueryUser(userid_t uid, uint8 * data, uint16 len) {
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (memcmp(iter->second.uid.x, uid.x, sizeof(userid_t)) == 0) {
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
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            CCardHost::GetInstance()->AckQueryUser(iter->second.uid, data, len);
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

void CValveMonitor::Recharge(userid_t uid, uint8 * data, uint16 len) {
    DEBUG("Recharge user id ");
    hexdump(uid.x, sizeof(userid_t));
    DEBUG("Recharge data ");
    hexdump(data, len);
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        DEBUG("Iter user id ");
        hexdump(iter->second.uid.x, sizeof(userid_t));
        if (memcmp(iter->second.uid.x, uid.x, sizeof(userid_t)) == 0) {
            DEBUG("Found user id ");
            hexdump(iter->second.uid.x, sizeof(userid_t));
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
    DEBUG("Want Valve MAC: ");
    //hexdump(&vmac, sizeof(uint32));
    DEBUG("%02x %02x %02x %02x\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        DEBUG("Iter Valve MAC: ");
        //hexdump(&iter->second.vmac, sizeof(uint32));
        DEBUG("%02x %02x %02x %02x\n", iter->second.vmac & 0xFF, (iter->second.vmac >> 8) & 0xFF, (iter->second.vmac >> 16) & 0xFF, (iter->second.vmac >> 24) & 0xFF);
        if (iter->second.vmac == vmac) {
            DEBUG("Found Valve MAC: ");
            //hexdump(&iter->second.vmac, sizeof(uint32));
            DEBUG("%02x %02x %02x %02x\n", iter->second.vmac & 0xFF, (iter->second.vmac >> 8) & 0xFF, (iter->second.vmac >> 16) & 0xFF, (iter->second.vmac >> 24) & 0xFF);
            CCardHost::GetInstance()->AckRecharge(iter->second.uid, data, len);
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

uint16 CValveMonitor::ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len) {
    uint16 okay = 0;

    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
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
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
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
    if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
        for (map<uint32, valve_temperature_t>::iterator iter = valveTemperatures.begin(); iter != valveTemperatures.end(); iter ++) {
            bzero(buf, PKTLEN);
            ptr = 0;
            buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
            buf[ptr] = 1 + 1 + sizeof(valve_temperature_t); ptr ++;
            memcpy(buf + ptr, &iter->second, sizeof(valve_temperature_t)); ptr += sizeof(valve_temperature_t);
            CPortal::GetInstance()->InsertValveData(buf, ptr);
        }
    } else {
        for (map<uint32, valve_heat_t>::iterator iter = valveHeats.begin(); iter != valveHeats.end(); iter ++) {
            bzero(buf, PKTLEN);
            ptr = 0;
            buf[ptr] = VALVE_PACKET_FLAG; ptr ++;
            buf[ptr] = 1 + 1 + sizeof(valve_heat_t); ptr ++;
            memcpy(buf + ptr, &iter->second, sizeof(valve_heat_t)); ptr += sizeof(valve_heat_t);
            CPortal::GetInstance()->InsertValveData(buf, ptr);
        }
    }
}

void CValveMonitor::GetHeatData() {
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32)); ptr += sizeof(uint32);
            buf[ptr] = PORT; ptr ++;
            buf[ptr] = RAND; ptr ++;
            buf[ptr] = 0x11; ptr ++; // len
            buf[ptr] = VALVE_GET_HEAT_DATA; ptr ++;
            buf[ptr] = 0x68; ptr ++;
            buf[ptr] = 0x20; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0xAA; ptr ++;
            buf[ptr] = 0x01; ptr ++;
            buf[ptr] = 0x03; ptr ++;
            buf[ptr] = 0x90; ptr ++;
            buf[ptr] = 0x1F; ptr ++;
            buf[ptr] = 0x0B; ptr ++;
            buf[ptr] = 0xEC; ptr ++;
            buf[ptr] = 0x16; ptr ++;
            cbuffer_write_done(tx);
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseHeatData(uint32 vmac, uint8 * data, uint16 len) {
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            uint16 ptr = 0;
            while (ptr < len && data[ptr] != 0x68) ptr ++; // skip preface
            if (ptr == len) {
                DEBUG("Invalid response from heat device\n");
                return;
            }

            ptr ++; // skip frame start flag

            uint8 type = data[ptr]; ptr ++;
            uint8 addr[7];
            memcpy(addr, data + ptr, 7); ptr += 7;
            uint8 code = data[ptr]; ptr ++;
            uint8 dlen = data[ptr]; ptr ++;
            uint16 identifier;
            memcpy(&identifier, data + ptr, 2); ptr += 2;
            uint8 sn = data[ptr]; ptr ++;
            uint8 dayHeat[5], heat[5], heatPower[5], flow[5], totalFlow[5];
            memcpy(dayHeat, data + ptr, 5); ptr += 5;
            memcpy(heat, data + ptr, 5); ptr += 5;
            memcpy(heatPower, data + ptr, 5); ptr += 5;
            memcpy(flow, data + ptr, 5); ptr += 5;
            memcpy(totalFlow, data + ptr, 5); ptr += 5;
            uint8 inTemp[3], outTemp[3], workTime[3];
            memcpy(inTemp, data + ptr, 3); ptr += 3;
            memcpy(outTemp, data + ptr, 3); ptr += 3;
            memcpy(workTime, data + ptr, 3); ptr += 3;

            // date
            uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
            uint32 utc = DateTime2TimeStamp(year, month, day, hour, minute, second);

            // device status
            uint16 status;
            memcpy(&status, data + ptr, 2); ptr += 2;

            if (valveHeats.count(vmac) == 0) {
                valve_heat_t tmp;
                tmp.mac = vmac;
                tmp.uid = iter->second.uid;
                tmp.type = type;
                memcpy(tmp.addr, addr, 7);
                memcpy(tmp.inTemp, inTemp, 3);
                memcpy(tmp.outTemp, outTemp, 3);
                memcpy(tmp.totalFlow, totalFlow, 5);
                memcpy(tmp.totalHeat, heat, 5);
                memcpy(tmp.workTime, workTime, 3);
                tmp.timestamp = utc;
                valveHeats[vmac] = tmp;
            } else {
                valveHeats[vmac].mac = vmac;
                valveHeats[vmac].uid = iter->second.uid;
                valveHeats[vmac].type = type;
                memcpy(valveHeats[vmac].addr, addr, 7);
                memcpy(valveHeats[vmac].inTemp, inTemp, 3);
                memcpy(valveHeats[vmac].outTemp, outTemp, 3);
                memcpy(valveHeats[vmac].totalFlow, totalFlow, 5);
                memcpy(valveHeats[vmac].totalHeat, heat, 5);
                memcpy(valveHeats[vmac].workTime, workTime, 3);
                valveHeats[vmac].timestamp = utc;
            }
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

bool CValveMonitor::SendCommand(uint8 * data, uint16 len) {
    fd_set wfds;
    struct timeval tv;
    int retval, w, wrote = 0;
    uint8 * buf = data;
    uint16 wlen = 0;
    uint8 retry = 3;

    FD_ZERO(&wfds);
    FD_SET(com, &wfds);

    tv.tv_sec = 5;
    tv.tv_usec = 0;

    while (retry > 0) {
        retry --;
        retval = select(com + 1, NULL, &wfds, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &wfds)) {
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
                    wrote = 0;
                    sleep(1);
                    return true;
                }
            }
        }
    }

    return false;
}

bool CValveMonitor::WaitCmdAck(uint8 * data, uint16 * len) {
    fd_set rfds;
    struct timeval tv;
    int retval, r, readed = 0;
    uint8 * ack = data;
    uint16 rlen = 0;
    uint8 retry = 3;

    FD_ZERO(&rfds);
    FD_SET(com, &rfds);

    while (retry > 0) {
        retry --;
        tv.tv_sec = 5;
        tv.tv_usec = 0;
        retval = select(com + 1, &rfds, NULL, NULL, &tv);
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
                    * len = rlen;
                    return true;
                }
            }
        }
    }
    return false;
}

void CValveMonitor::LoadRecords() {
    LOGDB * db = dbopen((char *)"data/records", DB_RDONLY, genrecordkey);
    if (db != NULL) {
        dbseq(db, vrecordseq, &records, sizeof(records));
        dbclose(db);
    }
}

void CValveMonitor::SaveRecords() {
    LOGDB * db = dbopen((char *)"data/records", DB_NEW, genrecordkey);
    if (db != NULL) {
        for (map<uint32, record_t>::iterator iter = records.begin(); iter != records.end(); iter ++) {
            dbput(db, (void *)&iter->first, sizeof(uint32), &iter->second, sizeof(record_t));
        }
        dbclose(db);
        syncUsers = false;
    }
}

void CValveMonitor::GetValveTime(uint32 vmac) {
    DEBUG("Get time of valve (%02x %02x %02x %02x)\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        memcpy(buf, &vmac, sizeof(uint32)); ptr += sizeof(uint32);
        buf[ptr] = PORT; ptr ++;
        buf[ptr] = RAND; ptr ++;
        buf[ptr] = 0x01; ptr ++; // len
        buf[ptr] = VALVE_GET_TIME; ptr ++;
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::ParseValveTime(uint32 vmac, uint8 * data, uint16 len) {
    DEBUG("Valve(%02x %02x %02x %02x) ", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    hexdump(data, len);
    uint8 ptr = 0;
    uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F); ptr ++;
    uint32 utc = DateTime2TimeStamp(year, month, day, hour, minute, second);
    uint32 now = 0;
    if (GetLocalTimeStamp(now)) {
        now += 8*60*60; // convert to be Beijing Time
        DEBUG("Time now: %d, valve: %d, cportal time ready? %s\n", now, utc, CPortal::GetInstance()->timeReady? "true": "false");
        if (CPortal::GetInstance()->timeReady && (now - utc > 60 || utc - now > 60)) {
            tm t;
            if (GetLocalTime(t, now))
                SetValveTime(vmac, &t);
        }
    }
}

void CValveMonitor::SetValveTime(uint32 vmac, tm * time) {
    DEBUG("Sync valve(%02x %02x %02x %02x) time to %d-%d-%d %d:%d:%d %d\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF, time->tm_year + 1900, time->tm_mon + 1, time->tm_mday, time->tm_hour, time->tm_min, time->tm_sec, time->tm_wday);
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        memcpy(buf, &vmac, sizeof(uint32)); ptr += sizeof(uint32);
        buf[ptr] = PORT; ptr ++;
        buf[ptr] = RAND; ptr ++;
        buf[ptr] = 0x09; ptr ++; // len
        buf[ptr] = VALVE_SET_TIME; ptr ++;
        uint16 century = ((time->tm_year + 1900) / 100);
        uint16 year = (time->tm_year + 1900) % 100;
        buf[ptr] = DEC2BCD(century); ptr ++;
        buf[ptr] = DEC2BCD(year); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_mon + 1); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_mday); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_hour); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_min); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_sec); ptr ++;
        buf[ptr] = DEC2BCD(time->tm_wday); ptr ++;
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::SyncValveTime() {
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        GetValveTime(iter->second.vmac);
    }
}

void CValveMonitor::SendErrorValves() {
    bool found = false;
    for (map<uint32, user_t>::iterator i = lastUsers.begin(); i != lastUsers.end(); i ++) {
        found = false;
        for (map<uint32, user_t>::iterator j = users.begin(); j != users.end(); j ++) {
            if (i->first == j->first) {
                found = true;
                break;
            }
        }
        if (!found) {
            // todo: send it to server
        }
    }
}
