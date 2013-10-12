#include "CValveMonitor.h"
#include "Utils.h"
#include "logdb.h"
#include "libs_emsys_odm.h"
#include "CPortal.h"
#include "CCardHost.h"
#include "csvparser.h"
#include <iostream>
#include <fstream>
#include <sys/epoll.h>

#ifdef DEBUG_VALVE
#include <time.h>
#define DEBUG(...) do {printf("%ld %s::%s %d ----", time(NULL), __FILE__, __func__, __LINE__);printf(__VA_ARGS__);} while(false)
#include "hexdump.h"
#else
#define DEBUG(...)
#ifndef hexdump
#define hexdump(data, len)
#endif
#endif

#define INTERVAL1 00 * 1000
#define INTERVAL2 00 * 1000

#define PKTLEN 512
#define PORT 0x0F
#define RAND 0xFB

#define VALVE_PACKET_FLAG 0xF1

void vuserseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen)
{
    map<uint32, user_t> * users = (map<uint32, user_t> *) params;
    uint32 * vmac = (uint32 *) key;
    user_t * user = (user_t *) data;
    (* users)[* vmac] = * user;
    //DEBUG("Load user for vmac: %02x %02x %02x %02x\n", * vmac & 0xFF, (* vmac >> 8) & 0xFF, (* vmac >> 16) & 0xFF, (* vmac >> 24) & 0xFF);
}

void vrecordseq(void * key, size_t klen, void * data, size_t dlen, void * params, size_t plen)
{
    map<uint32, record_t> * records = (map<uint32, record_t> *) params;
    uint32 * vmac = (uint32 *) key;
    record_t * record = (record_t *) data;
    (* records)[* vmac] = * record;
}

void recharge_callback(user_t * user)
{
    CCardHost::GetInstance()->AckRecharge(user->uid, NULL, 0);
}

CValveMonitor * CValveMonitor::instance = NULL;
CValveMonitor * CValveMonitor::GetInstance()
{
    if (instance == NULL) {
        instance = new CValveMonitor();
    }
    return instance;
}

CValveMonitor::CValveMonitor():noonTimer(DAY_TYPE), sid(0), counter(0), unregisteredCounter(0)
{
    tx = cbuffer_create(12, PKTLEN);
    htx = cbuffer_create(10, PKTLEN);
    rx = cbuffer_create(10, sizeof(uint32)); // valve mac
    if (access("data/", F_OK) == -1) {
        mkdir("data", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    }
}

CValveMonitor::~CValveMonitor()
{
    cbuffer_free(tx);
    cbuffer_free(htx);
    cbuffer_free(rx);
}

bool CValveMonitor::Init(uint32 startTime, uint32 interval)
{
    return punctualTimer.Start(interval * 60) && noonTimer.Start(startTime);
}

void CValveMonitor::LoadUsers()
{
    LOGDB * db = dbopen((char *)"data/users", DB_RDONLY, genuserkey);
    if (db != NULL) {
        dbseq(db, vuserseq, &lastUsers, sizeof(lastUsers));
        dbclose(db);
    }
}

void CValveMonitor::SaveUsers()
{
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

uint32 CValveMonitor::Run()
{
    struct timeval lastIO;
    int r, readed = 0, lastReaded = 0, w, wrote = 0;
    uint8 ack[PKTLEN];
    uint8 * buf = NULL, ptr = 0;;
    uint16 rlen = 0, wlen = 0;
    time_t lastWrite = 0;
    uint32 rc = 0, wc = 0; // read and write counter
    uint8 retry = 0; // read retry times
    uint8 priority = 0; // 0 for normal, 1 for high
    bool broadcasting = true;
    // int iomode = 0; // 1 for read, 0 for write
    int workmode = 0; // 0 for dynamical register, 1 for csv register
    int nfds = 0;

    struct epoll_event ev, evs[1];

    while ((r = read(com, buf, PKTLEN)) > 0) {
        readed += r;
    }
    DEBUG("Clear tty %d bytes\n", readed);
    readed = 0;

    int epfd = epoll_create(1);

    if (epfd == -1) {
        DEBUG("Cannot create epoll\n");
        return (uint32)-1;
    }

#if 0
    workmode = 0;
    broadcasting = false;
#else
    if (access("user_info.csv", F_OK) == 0) {
        workmode = 1;
    }

    LoadUsers();
    LoadRecords();
#endif

registering:

    if (workmode == 0) {
        BroadcastClearSID();
        sleep(1);
        BroadcastClearSID();
        sleep(1);
        BroadcastClearSID();
        sleep(1);

        srandom(time(NULL));
        sid = random();
    } else {
        vector<user_t> unregistered;
        vector<string> row;
        string line;
        ifstream in("user_info.csv");
        if (in.fail()) {
            DEBUG("user_info.csv not found\n");
            workmode = 0;
            goto registering;
        }
        while (getline(in, line) && in.good()) {
            user_t u;
            csvline_populate(row, line, ',');
            if (row.size() < 2) continue;
            if (row[0].length() != 8) continue;
            if (row[1].length() != 16) continue;
            if (str2hex((char *)row[0].c_str(), (uint8 *)&u.vmac) == 0) u.vmac = 0;
            if (str2hex((char *)row[1].c_str(), u.uid.x) == 0) bzero(u.uid.x, sizeof(userid_t));
            if (SetUID(u.vmac, u.uid)) {
                // found it
                users[u.vmac] = u;
            } else {
                unregistered.push_back(u);
            }
        }
        in.close();
        if (unregistered.size() > 0) {
            // report to server
        }
        if (users.size() == 0) {
            workmode = 0;
            goto registering;
        }
    }

    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = com;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, com, &ev) == -1) {
        DEBUG("Cannot epoll fd %d\n", com);
        return (uint32)-1;
    }

    while (true) {
        if (!broadcasting) {
            // check expired valve, only works in normal process
            time_t now = time(NULL);
            for (map<uint32, expire_t>::iterator iter = expires.begin(); iter != expires.end(); iter ++) {
                if (now > iter->second.timestamp) {
                    DEBUG("Recharge Valve[%02x %02x %02x %02x] is expired!\n", iter->first & 0xFF, (iter->first >> 8) & 0xFF, (iter->first >> 16) & 0xFF, (iter->first >> 24) & 0xFF);
                    iter->second.callback(&users[iter->first]);
                    expires.erase(iter++);
                }
            }
        }

        if (broadcasting && workmode == 0) {
            if (counter == 4) {
                unregisteredCounter = 0;
                BroadcastQuery();
                broadcasting = false;
                counter ++;
            } else {
                if ((unsigned int)(time(NULL) - lastWrite) > (120 + valveCount - 60)) {
                    while (cbuffer_read(rx) != NULL) {
                        ptr = 0;
                        buf = (uint8 *)cbuffer_write(htx);
                        if (buf != NULL) {
                            bzero(buf, PKTLEN);
                            memcpy(buf, cbuffer_read(rx), sizeof(uint32));
                            ptr += sizeof(uint32);
                            cbuffer_read_done(rx);
                            buf[ptr ++] = PORT;
                            buf[ptr ++] = RAND;
                            buf[ptr ++] = 0x06; //len
                            buf[ptr ++] = VALVE_BROADCAST;
                            buf[ptr ++] = 0xAC;
                            memcpy(buf + ptr, &sid, sizeof(uint32));
                            ptr += sizeof(uint32);
                            cbuffer_write_done(htx);
                        } else {
                            goto ioloopstart;
                        }
                        // SendCommand(ack, ptr); // send only
                    }
                    Broadcast(++counter);
                }
            }
        } else if (broadcasting == false && unregisteredCounter > 0 && workmode == 0) {
            DEBUG("%d valves are unregistered, rebroadcasting\n", unregisteredCounter);
            broadcasting = true;
            counter = 1;
        } else {
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
                if (workmode == 0) BroadcastQuery();
                truncate("realusers.csv", 0);
                int fd = open("realusers.csv", O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
                if (fd != -1) {
                    DEBUG("Save real users\n");
                    char buf[32];
                    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
                        bzero(buf, 32);
                        sprintf(buf, "%02X%02X%02X%02X,%02X%02X%02X%02X%02X%02X%02X%02X\r\n", iter->first & 0xFF, (iter->first >> 8) & 0xFF, (iter->first >> 16) & 0xFF, (iter->first >> 24) & 0xFF, iter->second.uid.x[0], iter->second.uid.x[1], iter->second.uid.x[2], iter->second.uid.x[3], iter->second.uid.x[4], iter->second.uid.x[5], iter->second.uid.x[6], iter->second.uid.x[7]);
                        write(fd, buf, strlen(buf));
                    }
                    close(fd);
                } else {
                    DEBUG("Save real users error\n");
                }

                truncate("statistics.txt", 0);
                fd = open("statistics.txt", O_WRONLY);
                if (fd != -1) {
                    DEBUG("Save statistics\n");
                    char buf[32];
                    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
                        bzero(buf, 32);
                        sprintf(buf, "%02X %02X %02X %02X %6d %6d %02.02f%%\n", iter->first & 0xFF, (iter->first >> 8) & 0xFF, (iter->first >> 16) & 0xFF, (iter->first >> 24) & 0xFF, responseCounters[iter->first], requestCounters[iter->first], responseCounters[iter->first] * (double)100 / requestCounters[iter->first]);
                        write(fd, buf, strlen(buf));
                    }
                    close(fd);
                } else {
                    DEBUG("Save statistics error\n");
                }
            }
            if (noonTimer.Done()) {
                GetRechargeData();
                GetConsumeData();
            }
        }

    ioloopstart:

        if ((cbuffer_read(tx) != NULL || cbuffer_read(htx) != NULL) && rc == wc) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            if (difftimeval(&tv, &lastIO) > 50 * 1000) {
                ev.events |= EPOLLOUT;
                if (epoll_ctl(epfd, EPOLL_CTL_MOD, com, &ev) == -1) {
                    DEBUG("Cannot modify epoll to listen EPULLOUT\n");
                }
            }
        }

        nfds = epoll_wait(epfd, evs, 1, 200);

        if (nfds > 0) {
            if (evs[0].events & EPOLLIN) {

                if (readed == 0) {
                    rlen = 0;
                    bzero(ack, PKTLEN);
                }

                while ((r = read(evs[0].data.fd, ack + readed, PKTLEN - readed)) > 0) {
                    readed += r;
                }
                gettimeofday(&lastIO, NULL);

                if (r == -1) {
                    if (errno != EAGAIN) {
                        DEBUG("Read error: %d\n", errno);
                        continue;
                    }
                    if (retry > 10) {
                        DEBUG("Read timeout, retry: %d, rc: %d, wc: %d, readed: %d\n", retry, rc, wc, readed);
                        if (readed > 0)
                            hexdump(ack, readed);

                        // try to recover from the wrong packet
                        ptr = 0;
                        while (readed > 5 && ack[ptr + 4] != 0x0F) {
                            ptr ++;
                            readed --;
                        }
                        if (ptr > 0) {
                            // found mistake in packet
                            memmove(ack, ack + ptr, readed);
                            DEBUG("Recover to ");
                            hexdump(ack, readed);
                        }

                        if (readed > 6 && ack[6] != 0xFF) {
                            rlen = 4 + 1 + 1 + 1 + ack[6] + 2; // mac + port + rand + len + data + crc
                        } else if (readed > 8 && ack[6] == 0xFF) {
                            rlen = 4 + 1 + 1 + 3 + ((uint16)ack[7]) << 8 + ack[8] + 2; // mac + port + rand + len + data + crc
                        } else {
                            rlen = 0;
                        }

                        if (readed >= rlen) {
                            // it's correct
                            goto adjust_read_packet;
                        }

                        readed = 0;
                        retry = 0;
                        rlen = 0;
                        if (rc > wc)
                            wc = rc;
                        else
                            rc = wc;
                        continue;
                    } else {
                        if (readed == lastReaded)
                            retry ++;
                        if (readed == 0) {
                            DEBUG(" break the read loop\n");
                            continue;
                        }
                        lastReaded = readed;
                    }
                }

                // try to recover from the wrong packet
                ptr = 0;
                while (readed > 5 && ack[ptr + 4] != 0x0F) {
                    ptr ++;
                    readed --;
                }
                if (ptr > 0) {
                    // found mistake in packet
                    memmove(ack, ack + ptr, readed);
                    DEBUG("Recover to ");
                    hexdump(ack, readed);
                }
            adjust_read_packet:
                bool cont = false;
                do {
                    if (readed > 6 && ack[6] != 0xFF) {
                        rlen = 4 + 1 + 1 + 1 + ack[6] + 2; // mac + port + rand + len + data + crc
                    } else if (readed > 8 && ack[6] == 0xFF) {
                        rlen = 4 + 1 + 1 + 3 + ((uint16)ack[7]) << 8 + ack[8] + 2; // mac + port + rand + len + data + crc
                    }
                    if (readed >= rlen && rlen > 0) {
                        DEBUG("Read %d bytes: ", rlen);
                        hexdump(ack, rlen);
                        ParseAck(ack, rlen);
                        rc ++;
                        if (readed > rlen) {
                            memmove(ack, ack + rlen, (readed - rlen));
                            readed -= rlen;
                        } else {
                            readed = 0;
                        }
                        lastReaded = readed;
                        retry = 0;
                        rlen = 0;
                        cont = true;
                    } else {
                        cont = false;
                    }
                } while (cont);
            }
            if (evs[0].events & EPOLLOUT) {
                buf = (uint8 *) cbuffer_read(htx);
                priority = 1;
                if (buf == NULL) {
                    buf = (uint8 *) cbuffer_read(tx);
                    if (buf == NULL) continue;
                    priority = 0;
                }
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
                w = write(evs[0].data.fd, buf + wrote, wlen - wrote);
                gettimeofday(&lastIO, NULL);
                if (w == -1 || w == 0) {
                    continue;
                }
                wrote += w;
                if (wrote == wlen) {
                    lastWrite = time(NULL);
#ifdef DEBUG_VALVE_TRACE_RECHARGE
                    uint8 code;
                    if (buf[6] == 0xFF) {
                        code = buf[9];
                    } else {
                        code = buf[7];
                    }
                    if (code == VALVE_RECHARGE) {
                        uint32 vmac;
                        memcpy(&vmac, buf, sizeof(uint32));
                        timers[vmac].timestamp2 = lastWrite;
                    }
#endif
                    DEBUG("Write %d bytes: ", wlen);
                    hexdump(buf, wlen);
                    if (priority == 0)
                        cbuffer_read_done(tx);
                    else
                        cbuffer_read_done(htx);
                    wrote = 0;
                    //myusleep(500 * 1000);
                    wc ++;
                    ev.events &= ~EPOLLOUT;
                    if (epoll_ctl(epfd, EPOLL_CTL_MOD, com, &ev) == -1) {
                        DEBUG("Cannot modify epoll to unlisten EPOLLOUT\n");
                    }
                }
            }
        } else {
            if (retry > 5) {
                //DEBUG("Poll timeout, retry: %d, rc: %d, wc: %d\n", retry, rc, wc);
                if (rc > wc)
                    wc = rc;
                else
                    rc = wc;
                rlen = 0;
                readed = 0;
                retry = 0;
            } else {
                retry ++;
            }
        }
    }

    return 0;
}

void CValveMonitor::BroadcastClearSID()
{
    uint8 buf[PKTLEN];
    uint8 ptr = 0;
    bzero(buf, PKTLEN);
    buf[ptr ++] = 0xFF;
    buf[ptr ++] = 0xFF;
    buf[ptr ++] = 0xFF;
    buf[ptr ++] = 0xFF;
    buf[ptr ++] = PORT;
    buf[ptr ++] = RAND;
    buf[ptr ++] = 0x01;
    buf[ptr ++] = VALVE_BROADCAST_CLEAR_SID;
    uint16 crc = GenerateCRC(buf, ptr);
    buf[ptr ++] = (crc >> 8) & 0xFF;
    buf[ptr ++] = crc & 0xFF;
    ptr = write(com, buf, ptr);
    DEBUG("Write %d bytes ", ptr);
    hexdump(buf, ptr);
    //SendCommand(buf, ptr);
}

void CValveMonitor::Broadcast(uint8 counter)
{
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = PORT;
        ptr ++;
        buf[ptr] = RAND;
        ptr ++;
        buf[ptr] = 0x06;
        ptr ++;
        buf[ptr] = VALVE_BROADCAST;
        ptr ++;
        buf[ptr] = counter;
        ptr ++;
        memcpy(buf + ptr, &sid, sizeof(uint32));
        ptr += sizeof(uint32);
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::BroadcastQuery()
{
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = 0xFF;
        ptr ++;
        buf[ptr] = PORT;
        ptr ++;
        buf[ptr] = RAND;
        ptr ++;
        buf[ptr] = 0x06;
        ptr ++;
        buf[ptr] = VALVE_BROADCAST;
        ptr ++;
        buf[ptr] = 0xCC;
        ptr ++;
        memcpy(buf + ptr, &sid, sizeof(uint32));
        ptr += sizeof(uint32);
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::ParseBroadcast(uint32 vmac, uint8 * data, uint16 len)
{
    uint8 * buf;
    uint16 ptr = 0;
    user_t user;
    if (len == 8) {
        memcpy(user.uid.x, data, sizeof(userid_t));
        map<uint32, user_t>::iterator i = users.find(vmac);
        if (i != users.end()) {
            DEBUG("Valve [%02x %02x %02x %02x] has bound user [%02x %02x %02x %02x %02x %02x %02x %02x], but still tries to bind user [%02x %02x %02x %02x %02x %02x %02x %02x], total count: %d\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF, i->second.uid.x[0], i->second.uid.x[1], i->second.uid.x[2], i->second.uid.x[3], i->second.uid.x[4], i->second.uid.x[5], i->second.uid.x[6], i->second.uid.x[7], user.uid.x[0], user.uid.x[1], user.uid.x[2], user.uid.x[3], user.uid.x[4], user.uid.x[5], user.uid.x[6], user.uid.x[7], users.size());
            // we still need to response it
        } else {
            user.vmac = vmac;
            users[vmac] = user;
            DEBUG("Found user [%02x %02x %02x %02x %02x %02x %02x %02x] in valve [%02x %02x %02x %02x], total count: %d\n", user.uid.x[0], user.uid.x[1], user.uid.x[2], user.uid.x[3], user.uid.x[4], user.uid.x[5], user.uid.x[6], user.uid.x[7], user.vmac & 0xFF, (user.vmac >> 8) & 0xFF, (user.vmac >> 16) & 0xFF, (user.vmac >> 24) & 0xFF, users.size());
            syncUsers = true;
        }
        buf = (uint8 *) cbuffer_write(rx);
        if (buf != NULL) {
            DEBUG("Add valve [%02x %02x %02x %02x] to response queue\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
            bzero(buf, sizeof(uint32));
            memcpy(buf, &vmac, sizeof(uint32));
            cbuffer_write_done(rx);
        } else {
            buf = (uint8 *) cbuffer_write(tx);
            if (buf != NULL) {
                DEBUG("Response queue is full, then add valve [%02x %02x %02x %02x] to sending command buffer directly\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
                bzero(buf, PKTLEN);
                memcpy(buf, &vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr] = PORT;
                ptr ++;
                buf[ptr] = RAND;
                ptr ++;
                buf[ptr] = 0x06;
                ptr ++; // len
                buf[ptr] = VALVE_BROADCAST;
                ptr ++;
                buf[ptr] = 0xAC;
                ptr ++;
                memcpy(buf + ptr, &sid, sizeof(uint32));
                ptr += sizeof(uint32);
                cbuffer_write_done(tx);
                //SendCommand(buf, ptr); // send only, no reponse
            } else {
                DEBUG("No enough memory to response valve [%02x %02x %02x %02x]\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
            }
        }
    } else if (len == 1) {
        map<uint32, user_t>::iterator i = users.find(vmac);
        if (i != users.end()) {
            DEBUG("Valve[%02x %02x %02x %02x] is unregistered, but we know it, total count: %d\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF, users.size());
        } else {
            DEBUG("Valve[%02x %02x %02x %02x] is unregistered, total count: %d\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF, users.size());
            bzero(user.uid.x, sizeof(userid_t));
            user.vmac = vmac;
            users[vmac] = user;

            txlock.Lock();
            uint8 * buf = (uint8 *) cbuffer_write(tx);
            if (buf != NULL) {
                uint16 ptr = 0;
                memcpy(buf, &vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr] = PORT;
                ptr ++;
                buf[ptr] = RAND;
                ptr ++;
                buf[ptr] = 0x01;
                ptr ++; // len
                buf[ptr] = VALVE_GET_PUNCTUAL_DATA;
                ptr ++;
                cbuffer_write_done(tx);
            }
            txlock.UnLock();
        }
        unregisteredCounter ++;
    }
}

bool CValveMonitor::GetUserList(vector<user_t>& u)
{
    if (users_lock.TryLock()) {
        for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
            u.push_back(iter->second);
        }
        users_lock.UnLock();
        return true;
    }
    return false;
}

void CValveMonitor::ParseAck(uint8 * ack, uint16 len)
{
    uint16 gcrc = GenerateCRC(ack, len - 2);
    if ((gcrc & 0xFF) != ack[len - 1] && ((gcrc >> 8) & 0xFF) != ack[len - 2]) {
        DEBUG("CRC error want %02x %02x, generate %02x %02x\n", ack[len - 1], ack[len - 2], (gcrc & 0xFF), ((gcrc >> 8) & 0xFF));
        return;
    }
    uint32 mac = 0;
    memcpy(&mac, ack, sizeof(uint32));
    /*
    DEBUG("Parsing Ack for valve[%02x %02x %02x %02x] ", mac & 0xFF, (mac >> 8) & 0xFF, (mac >> 16) & 0xFF, (mac >> 24) & 0xFF);
    hexdump(ack, len);
    */
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
        responseCounters[mac]++;
        ParseValveTime(mac, data, dlen);
        break;
    case VALVE_GET_RECHARGE_DATA:
        responseCounters[mac]++;
        ParseRechargeData(mac, data, dlen);
        break;
    case VALVE_GET_CONSUME_DATA:
        responseCounters[mac]++;
        ParseConsumeData(mac, data, dlen);
        break;
    case VALVE_GET_TIME_DATA:
        responseCounters[mac]++;
        ParseTimeData(mac, data, dlen);
        break;
    case VALVE_GET_HEAT_DATA:
        responseCounters[mac]++;
        ParseHeatData(mac, data, dlen);
        break;
    case VALVE_GET_PUNCTUAL_DATA:
        responseCounters[mac]++;
        ParsePunctualData(mac, data, dlen);
        break;
    case VALVE_QUERY_USER:
        responseCounters[mac]++;
        ParseQueryUser(mac, data, dlen);
        break;
    case VALVE_RECHARGE:
        responseCounters[mac]++;
        ParseRecharge(mac, data, dlen);
        break;
    case VALVE_BROADCAST:
        responseCounters[mac];
        ParseBroadcast(mac, data, dlen);
        break;
    case VALVE_PUNCTUAL_DATA_NOT_AVAILABLE:
        responseCounters[mac]++;
        DEBUG("Punctual data not avaiable in valve[%02x %02x %02x %02x]\n", mac & 0xFF, (mac >> 8) & 0xFF, (mac >> 16) & 0xFF, (mac >> 24) & 0xFF);
        break;
    default:
        DEBUG("Unknow ack from valve[%02x %02x %02x %02x]: ", mac & 0xFF, (mac >> 8) & 0xFF, (mac >> 16) & 0xFF, (mac >> 24) & 0xFF);
        hexdump(data, dlen);
        break;
    }
}

void CValveMonitor::GetPunctualData()
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        user_t * user = &iter->second;
        if (user->uid.x[0] == 0 && user->uid.x[1] == 0 && user->uid.x[2] == 0 && user->uid.x[3] == 0 && user->uid.x[4] == 0 && user->uid.x[5] == 0 && user->uid.x[6] == 0 && user->uid.x[7] == 0)
            continue;
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32));
            ptr += sizeof(uint32);
            buf[ptr ++] = PORT;
            buf[ptr ++] = RAND;
            buf[ptr ++] = 0x01; // len
            buf[ptr ++] = VALVE_GET_PUNCTUAL_DATA;
            cbuffer_write_done(tx);
            requestCounters[iter->second.vmac] ++;
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParsePunctualData(uint32 vmac, uint8 * data, uint16 len)
{

    uint16 ptr = 0;
    // transcation date
    uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint32 utc = DateTime2TimeStamp(year, month, day, hour, 0, 0);

    // temperatures
    uint16 avgTemp = (((uint16)data[ptr] << 8) & 0xFF00) | data[ptr];
    ptr += 2;
    uint16 avgSetTemp = (((uint16)data[ptr] << 8) & 0xFF00) | data[ptr];
    ptr += 2;

    // device open time
    uint8 openTime = data[ptr];
    ptr ++;

    // device status
    uint16 status;
    memcpy(&status, data + ptr, 2);
    ptr += 2;

    // uid
    userid_t uid;
    memcpy(&uid.x, data + ptr, sizeof(userid_t));
    ptr += sizeof(userid_t);

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
    year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    utc = DateTime2TimeStamp(year, month, day, hour, minute, second);

    if (records.count(vmac) == 0) {
        record_t rec;
        rec.recharge = data[ptr] - 1;
        ptr ++;
        rec.sentRecharge = 0;
        rec.consume = data[ptr] * 256 + data[ptr + 1] - 1;
        ptr += 2;
        rec.sentConsume = 0;
        rec.temperature = data[ptr] * 256 + data[ptr + 1] - 1;
        ptr += 2;
        rec.sentTemperature = 0;
        rec.status = data[ptr] - 1;
        ptr ++;
        rec.sentStatus = 0;
        records[vmac] = rec;
        syncRecords = true;
    } else {
        if (records[vmac].recharge != data[ptr] - 1) {
            syncRecords = true;
        }
        records[vmac].recharge = data[ptr] - 1;
        ptr ++;
        if (records[vmac].consume != data[ptr] * 256 + data[ptr + 1] - 1) {
            syncRecords = true;
        }
        records[vmac].consume = data[ptr] * 256 + data[ptr + 1] - 1;
        ptr += 2;
        records[vmac].temperature = data[ptr] * 256 + data[ptr + 1] - 1;
        ptr += 2;
        records[vmac].status = data[ptr] - 1;
        ptr ++;
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

void CValveMonitor::GetRechargeData()
{
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
                memcpy(buf, &iter->second.vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr ++] = PORT;
                buf[ptr ++] = RAND;
                buf[ptr ++] = 0x03; // len
                buf[ptr ++] = VALVE_GET_RECHARGE_DATA;
                buf[ptr ++] = rec->sentRecharge + i * payload + 1; // offset
                buf[ptr ++] = payload; // limit
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
                memcpy(buf, &iter->second.vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr ++] = PORT;
                buf[ptr ++] = RAND;
                buf[ptr ++] = 0x03; // len
                buf[ptr ++] = VALVE_GET_RECHARGE_DATA;
                buf[ptr ++] = rec->sentRecharge + (delta / payload) * payload + 1; // offset
                buf[ptr ++] = delta % payload; // limit

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

void CValveMonitor::ParseRechargeData(uint32 vmac, uint8 * data, uint16 len)
{
    uint8 buf[PKTLEN];
    const uint8 rsize = 42;
    uint16 ptr = 0;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            for (uint8 i = 0, count = len / rsize; i < count; i ++, ptr = 0) {
                bzero(buf, PKTLEN);
                buf[ptr ++] = VALVE_PACKET_FLAG;
                buf[ptr ++] = sizeof(userid_t) + rsize;
                memcpy(buf + ptr, iter->second.uid.x, sizeof(userid_t));
                ptr += sizeof(userid_t);
                * (uint32 *)(buf + ptr) = vmac;
                ptr += sizeof(uint32);
                memcpy(buf + ptr, data + i * rsize, rsize);
                ptr += rsize;
                CPortal::GetInstance()->InsertChargeData(buf, ptr);
            }
            return;
        }
    }
}

void CValveMonitor::GetConsumeData()
{
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
                memcpy(buf, &iter->second.vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr ++] = PORT;
                buf[ptr ++] = RAND;
                buf[ptr ++] = 0x04; // len
                buf[ptr ++] = VALVE_GET_CONSUME_DATA;
                buf[ptr ++] = (rec->sentConsume + i * payload + 1) / 256;
                buf[ptr ++] = (rec->sentConsume + i * payload + 1) % 256;
                buf[ptr ++] = payload;
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
                memcpy(buf, &iter->second.vmac, sizeof(uint32));
                ptr += sizeof(uint32);
                buf[ptr ++] = PORT;
                buf[ptr ++] = RAND;
                buf[ptr ++] = 0x04; // len
                buf[ptr ++] = VALVE_GET_CONSUME_DATA;
                buf[ptr ++] = (rec->sentConsume + (delta / payload) * payload + 1) / 256;
                buf[ptr ++] = (rec->sentConsume + (delta / payload) * payload + 1) % 256;
                buf[ptr ++] = delta % payload;
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

void CValveMonitor::ParseConsumeData(uint32 vmac, uint8 * data, uint16 len)
{
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    const uint8 rsize = 8;
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            for (uint8 i = 0, count = (len - 8) / rsize; i < count; i ++, ptr = 0) {
                bzero(buf, PKTLEN);
                buf[ptr ++] = VALVE_PACKET_FLAG;
                buf[ptr ++] = sizeof(userid_t) + rsize;
                * (uint32 *)(buf + ptr) = vmac;
                ptr += sizeof(uint32);
                memcpy(buf + ptr, iter->second.uid.x, sizeof(userid_t));
                ptr += sizeof(userid_t);
                memcpy(buf + ptr, data + i * rsize + 4, 4);
                ptr += 24; // used time unit and skip unset fields

                uint8 tmp[7] = {0};
                tmp[6] = data[i * rsize + 0];//yy
                tmp[5] = data[i * rsize + 1];//yy
                tmp[4] = data[i * rsize + 2];//mm
                tmp[3] = data[i * rsize + 3];//dd
                uint32 timestamp = DateTime2TimeStamp(tmp, 7);
                memcpy(buf + ptr, &timestamp, sizeof(uint32));
                ptr += sizeof(uint32);

                CPortal::GetInstance()->InsertConsumeData(buf, ptr);
            }
            return;
        }
    }
}

void CValveMonitor::QueryUser(userid_t uid, uint8 * data, uint16 len)
{
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
            memcpy(buf, &iter->second.vmac, sizeof(uint32));
            ptr += sizeof(uint32);
            buf[ptr ++] = PORT;
            buf[ptr ++] = RAND;
            if ((1 + len) < 0xFF) {
                buf[ptr ++] = (1 + len) & 0xFF; // command + data
                buf[ptr ++] = VALVE_QUERY_USER;
                memcpy(buf + ptr, data, len);
            } else {
                buf[ptr ++] = 0xFF;
                buf[ptr ++] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                buf[ptr ++] = (1 + len) & 0xFF; // low byte for 'command + data'
                buf[ptr ++] = VALVE_QUERY_USER;
                memcpy(buf + ptr, data, len);
            }
            cbuffer_write_done(tx);
            txlock.UnLock();
            return;
        }
    }
}

void CValveMonitor::ParseQueryUser(uint32 vmac, uint8 * data, uint16 len)
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            CCardHost::GetInstance()->AckQueryUser(iter->second.uid, data, len);
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

void CValveMonitor::Recharge(userid_t uid, uint8 * data, uint16 len)
{
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
            htxlock.Lock();
            uint8 * buf = (uint8 *)cbuffer_write(htx);
            if (buf == NULL) {
                DEBUG("No enough memory for recharge\n");
                CCardHost::GetInstance()->AckRecharge(uid, NULL, 0);
                return;
            }
            bzero(buf, PKTLEN);
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32));
            ptr += sizeof(uint32);
            buf[ptr ++] = PORT;
            buf[ptr ++] = RAND;
            if ((1 + len) < 0xFF) {
                buf[ptr ++] = (1 + len) & 0xFF; // command + data
                buf[ptr ++] = VALVE_RECHARGE;
                memcpy(buf + ptr, data, len);
            } else {
                buf[ptr ++] = 0xFF;
                buf[ptr ++] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
                buf[ptr ++] = (1 + len) & 0xFF; // low byte for 'command + data'
                buf[ptr ++] = VALVE_RECHARGE;
                memcpy(buf + ptr, data, len);
            }
            cbuffer_write_done(htx);
            htxlock.UnLock();
            expire_t expire;
            expire.timestamp = time(NULL) + 5; // 5 seconds
            expire.callback = recharge_callback;
            expires[iter->first] = expire;
#ifdef DEBUG_VALVE_TRACE_RECHARGE
            mytimer_t timer;
            timer.timestamp1 = time(NULL);
            timers[iter->first] = timer;
#endif
            return;
        }
    }
}

void CValveMonitor::ParseRecharge(uint32 vmac, uint8 * data, uint16 len)
{
#ifdef DEBUG_VALVE_TRACE_RECHARGE
    timers[vmac].timestamp3 = time(NULL);
    mytimer_t timer = timers[vmac];
    DEBUG("Recharge timing: received -> sent: %ld, sent -> acked: %ld\n", timer.timestamp2 - timer.timestamp1, timer.timestamp3 - timer.timestamp2);
#endif
    DEBUG("Want Valve MAC: %02x %02x %02x %02x\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        DEBUG("Iter Valve MAC: %02x %02x %02x %02x\n", iter->second.vmac & 0xFF, (iter->second.vmac >> 8) & 0xFF, (iter->second.vmac >> 16) & 0xFF, (iter->second.vmac >> 24) & 0xFF);
        if (iter->second.vmac == vmac) {
            map<uint32, expire_t>::iterator i = expires.find(vmac);
            if (i != expires.end()) {
                expires.erase(i);
            }
            DEBUG("Found Valve MAC: %02x %02x %02x %02x\n", iter->second.vmac & 0xFF, (iter->second.vmac >> 8) & 0xFF, (iter->second.vmac >> 16) & 0xFF, (iter->second.vmac >> 24) & 0xFF);
            CCardHost::GetInstance()->AckRecharge(iter->second.uid, data, len);
            return;
        }
    }
    DEBUG("No valve mac(%02x %02x %02x %02x) found\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
}

uint16 CValveMonitor::ConfigValve(ValveCtrlType cmd, uint8 * data, uint16 len)
{
    uint16 okay = 0;

    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        htxlock.Lock();
        uint8 * buf = (uint8 *)cbuffer_write(htx);
        uint16 ptr = 0;
        if (buf == NULL) {
            DEBUG("No enough memory for config valve\n");
            continue;
        }

        bzero(buf, PKTLEN);
        memcpy(buf, &iter->second.vmac, sizeof(uint32));
        ptr += sizeof(uint32);
        buf[ptr ++] = PORT;
        buf[ptr ++] = RAND;
        if ((1 + len) < 0xFF) {
            buf[ptr ++] = (1 + len) & 0xFF; // command + data
            buf[ptr ++] = cmd;
            memcpy(buf + ptr, data, len);
        } else {
            buf[ptr ++] = 0xFF;
            buf[ptr ++] = ((1 + len) >> 8) & 0xFF; // high byte for 'command + data'
            buf[ptr ++] = (1 + len) & 0xFF; // low byte for 'command + data'
            buf[ptr ++] = cmd;
            memcpy(buf + ptr, data, len);
        }
        cbuffer_write_done(htx);
        okay ++;
        htxlock.UnLock();
    }
    return okay;
}

Status CValveMonitor::GetStatus()
{
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

void CValveMonitor::GetTimeData()
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32));
            ptr += sizeof(uint32);
            buf[ptr ++] = PORT;
            buf[ptr ++] = RAND;
            buf[ptr ++] = 0x01; // len
            buf[ptr ++] = VALVE_GET_TIME_DATA;
            cbuffer_write_done(tx);
            requestCounters[iter->second.vmac] ++;
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseTimeData(uint32 vmac, uint8 * data, uint16 len)
{
    uint16 ptr = 0;
    if (valveTemperatures.count(vmac) == 0) {
        valve_temperature_t tmp;
        memcpy(&tmp.totalTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
        memcpy(&tmp.remainingTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
        memcpy(&tmp.runningTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
        valveTemperatures[vmac] = tmp;
    } else {
        memcpy(&valveTemperatures[vmac].totalTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
        memcpy(&valveTemperatures[vmac].remainingTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
        memcpy(&valveTemperatures[vmac].runningTime, data + ptr, sizeof(uint64));
        ptr += sizeof(uint64);
    }
}

void CValveMonitor::SendValveData()
{
    uint8 buf[PKTLEN];
    uint16 ptr = 0;
    if (valveDataType == VALVE_DATA_TYPE_TEMPERATURE) {
        for (map<uint32, valve_temperature_t>::iterator iter = valveTemperatures.begin(); iter != valveTemperatures.end(); iter ++) {
            bzero(buf, PKTLEN);
            ptr = 0;
            buf[ptr ++] = VALVE_PACKET_FLAG;
            buf[ptr ++] = 1 + 1 + sizeof(valve_temperature_t);
            memcpy(buf + ptr, &iter->second, sizeof(valve_temperature_t));
            ptr += sizeof(valve_temperature_t);
            CPortal::GetInstance()->InsertValveData(buf, ptr);
        }
    } else {
        for (map<uint32, valve_heat_t>::iterator iter = valveHeats.begin(); iter != valveHeats.end(); iter ++) {
            bzero(buf, PKTLEN);
            ptr = 0;
            buf[ptr ++] = VALVE_PACKET_FLAG;
            buf[ptr ++] = 1 + 1 + sizeof(valve_heat_t);
            memcpy(buf + ptr, &iter->second, sizeof(valve_heat_t));
            ptr += sizeof(valve_heat_t);
            CPortal::GetInstance()->InsertValveData(buf, ptr);
        }
    }
}

void CValveMonitor::GetHeatData()
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        txlock.Lock();
        uint8 * buf = (uint8 *) cbuffer_write(tx);
        if (buf != NULL) {
            uint16 ptr = 0;
            memcpy(buf, &iter->second.vmac, sizeof(uint32));
            ptr += sizeof(uint32);
            buf[ptr ++] = PORT;
            buf[ptr ++] = RAND;
            buf[ptr ++] = 0x11; // len
            buf[ptr ++] = VALVE_GET_HEAT_DATA;
            buf[ptr ++] = 0x68;
            buf[ptr ++] = 0x20;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0xAA;
            buf[ptr ++] = 0x01;
            buf[ptr ++] = 0x03;
            buf[ptr ++] = 0x90;
            buf[ptr ++] = 0x1F;
            buf[ptr ++] = 0x0B;
            buf[ptr ++] = 0xEC;
            buf[ptr ++] = 0x16;
            cbuffer_write_done(tx);
            requestCounters[iter->second.vmac] ++;
        }
        txlock.UnLock();
    }
}

void CValveMonitor::ParseHeatData(uint32 vmac, uint8 * data, uint16 len)
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        if (iter->second.vmac == vmac) {
            uint16 ptr = 0;
            while (ptr < len && data[ptr] != 0x68) ptr ++; // skip preface
            if (ptr == len) {
                DEBUG("Invalid response from heat device\n");
                return;
            }

            ptr ++; // skip frame start flag

            uint8 type = data[ptr ++];
            uint8 addr[7];
            memcpy(addr, data + ptr, 7);
            ptr += 7;
            uint8 code = data[ptr ++];
            uint8 dlen = data[ptr ++];
            uint16 identifier;
            memcpy(&identifier, data + ptr, 2);
            ptr += 2;
            uint8 sn = data[ptr ++];
            uint8 dayHeat[5], heat[5], heatPower[5], flow[5], totalFlow[5];
            memcpy(dayHeat, data + ptr, 5);
            ptr += 5;
            memcpy(heat, data + ptr, 5);
            ptr += 5;
            memcpy(heatPower, data + ptr, 5);
            ptr += 5;
            memcpy(flow, data + ptr, 5);
            ptr += 5;
            memcpy(totalFlow, data + ptr, 5);
            ptr += 5;
            uint8 inTemp[3], outTemp[3], workTime[3];
            memcpy(inTemp, data + ptr, 3);
            ptr += 3;
            memcpy(outTemp, data + ptr, 3);
            ptr += 3;
            memcpy(workTime, data + ptr, 3);
            ptr += 3;

            // date
            uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
            ptr ++;
            uint32 utc = DateTime2TimeStamp(year, month, day, hour, minute, second);

            // device status
            uint16 status;
            memcpy(&status, data + ptr, 2);
            ptr += 2;

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

bool CValveMonitor::SendCommand(uint8 * data, uint16 len)
{
    fd_set wfds;
    struct timeval tv;
    int retval, w, wrote = 0;
    uint8 * buf = data;
    uint16 wlen = 0;
    uint8 retry = 3;

    FD_ZERO(&wfds);
    FD_SET(com, &wfds);

    tv.tv_sec = 0;
    tv.tv_usec = 1000;

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
                    //myusleep(500 * 1000);
                    return true;
                }
            }
        }
    }
    DEBUG("Exceed 3 retries\n");

    return false;
}

bool CValveMonitor::WaitCmdAck(uint8 * data, uint16 * len)
{
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
        tv.tv_sec = 0;
        tv.tv_usec = 1000;
        retval = select(com + 1, &rfds, NULL, NULL, &tv);
        if (retval) {
            if (FD_ISSET(com, &rfds)) {

                if (readed == 0) {
                    rlen = 9;
                    bzero(ack, PKTLEN);
                }

                r = read(com, ack + readed, rlen - readed);

                if (r == -1) {
                    continue;
                }
                readed += r;

                if (rlen == 9 && readed > 6 && ack[6] != 0xFF) {
                    rlen = 4 + 1 + 1 + 1 + ack[6] + 2; // mac + port + rand + len + data + crc
                } else if (rlen == 9 && readed > 6 && ack[6] == 0xFF) {
                    rlen = 4 + 1 + 1 + 3 + ((uint16)ack[7]) << 8 + ack[8] + 2; // mac + port + rand + len + data + crc
                }
                if (readed == rlen) {
                    readed = 0;
                    DEBUG("Read %d bytes: ", rlen);
                    hexdump(ack, rlen);
                    * len = rlen;
                    return true;
                }
            }
        }
    }
    DEBUG("Exceed 3 retries\n");
    return false;
}

void CValveMonitor::LoadRecords()
{
    LOGDB * db = dbopen((char *)"data/records", DB_RDONLY, genrecordkey);
    if (db != NULL) {
        dbseq(db, vrecordseq, &records, sizeof(records));
        dbclose(db);
    }
}

void CValveMonitor::SaveRecords()
{
    LOGDB * db = dbopen((char *)"data/records", DB_NEW, genrecordkey);
    if (db != NULL) {
        for (map<uint32, record_t>::iterator iter = records.begin(); iter != records.end(); iter ++) {
            dbput(db, (void *)&iter->first, sizeof(uint32), &iter->second, sizeof(record_t));
        }
        dbclose(db);
        syncRecords = false;
    }
}

void CValveMonitor::GetValveTime(uint32 vmac)
{
    DEBUG("Get time of valve (%02x %02x %02x %02x)\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        memcpy(buf, &vmac, sizeof(uint32));
        ptr += sizeof(uint32);
        buf[ptr ++] = PORT;
        buf[ptr ++] = RAND;
        buf[ptr ++] = 0x01; // len
        buf[ptr ++] = VALVE_GET_TIME;
        cbuffer_write_done(tx);
        requestCounters[vmac] ++;
    }
    txlock.UnLock();
}

void CValveMonitor::ParseValveTime(uint32 vmac, uint8 * data, uint16 len)
{
    DEBUG("Valve(%02x %02x %02x %02x) ", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF);
    hexdump(data, len);
    uint8 ptr = 0;
    uint16 year = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    year = year * 100 + ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 month = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 day = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 hour = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 minute = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint8 second = ((data[ptr] & 0xF0) >> 4) * 10 + (data[ptr] & 0x0F);
    ptr ++;
    uint32 utc = DateTime2TimeStamp(year, month, day, hour, minute, second);
    uint32 now = 0;
    if (GetLocalTimeStamp(now)) {
        now += 8*60*60; // convert to be Beijing Time
        DEBUG("Time now: %d, valve: %d, portal time ready? %s\n", now, utc, CPortal::GetInstance()->timeReady? "true": "false");
        if (CPortal::GetInstance()->timeReady && (now - utc > 60 || utc - now > 60)) {
            tm t;
            if (GetLocalTime(t, now))
                SetValveTime(vmac, &t);
        }
    }
}

void CValveMonitor::SetValveTime(uint32 vmac, tm * t)
{
    DEBUG("Sync valve(%02x %02x %02x %02x) time to %d-%d-%d %d:%d:%d %d\n", vmac & 0xFF, (vmac >> 8) & 0xFF, (vmac >> 16) & 0xFF, (vmac >> 24) & 0xFF, t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec, t->tm_wday);
    txlock.Lock();
    uint8 * buf = (uint8 *) cbuffer_write(tx);
    if (buf != NULL) {
        uint16 ptr = 0;
        memcpy(buf, &vmac, sizeof(uint32));
        ptr += sizeof(uint32);
        buf[ptr ++] = PORT;
        buf[ptr ++] = RAND;
        buf[ptr ++] = 0x09; // len
        buf[ptr ++] = VALVE_SET_TIME;
        uint16 century = ((t->tm_year + 1900) / 100);
        uint16 year = (t->tm_year + 1900) % 100;
        buf[ptr ++] = DEC2BCD(century);
        buf[ptr ++] = DEC2BCD(year);
        buf[ptr ++] = DEC2BCD(t->tm_mon + 1);
        buf[ptr ++] = DEC2BCD(t->tm_mday);
        buf[ptr ++] = DEC2BCD(t->tm_hour);
        buf[ptr ++] = DEC2BCD(t->tm_min);
        buf[ptr ++] = DEC2BCD(t->tm_sec);
        buf[ptr ++] = DEC2BCD(t->tm_wday);
        cbuffer_write_done(tx);
    }
    txlock.UnLock();
}

void CValveMonitor::SyncValveTime()
{
    for (map<uint32, user_t>::iterator iter = users.begin(); iter != users.end(); iter ++) {
        GetValveTime(iter->second.vmac);
    }
}

void CValveMonitor::SendErrorValves()
{
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

bool CValveMonitor::SetUID(uint32 vmac, userid_t uid)
{
    uint8 cmd[PKTLEN], ack[PKTLEN];
    bzero(cmd, PKTLEN);
    bzero(ack, PKTLEN);
    uint16 ptr = 0;
    uint16 tmplen;
    memcpy(cmd, &vmac, sizeof(uint32));
    ptr += sizeof(uint32);
    cmd[ptr ++] = PORT;
    cmd[ptr ++] = RAND;
    cmd[ptr ++] = 0x09; // len
    cmd[ptr ++] = VALVE_SET_UID;
    memcpy(cmd + ptr, uid.x, sizeof(userid_t));
    ptr += sizeof(userid_t);
    hexdump(cmd, ptr);
    for (int i = 0; i < 3; i ++) {
        SendCommand(cmd, ptr);
        //sleep(1);
        if (WaitCmdAck(ack, &tmplen)) {
            if (memcmp(ack, &vmac, sizeof(uint32)) == 0 && ack[7] == VALVE_SET_UID) {
                // found it
                return true;
            }
        }
    }
    return false;
}
