#ifndef __CCARDHOST_H
#define __CCARDHOST_H
#include "IThread.h"
#include "cbuffer.h"
#include "CLock.h"

class CCardHost: public IThread {

public:
    static CCardHost * GetInstance();
    void SetCom(int com) {this->com = com;};
    void AckQueryUser(userid_t uid, uint8 * data, uint16 len); // called by IValveMonitor
    void AckRecharge(userid_t uid, uint8 * data, uint16 len); // called by IValveMonitor
    void AckTimeOrRemove(uint8 * data, uint16 len);

private:
    CCardHost();
    ~CCardHost();
    virtual uint32 Run();
    void ParseAndExecute(uint8 * cmd, uint16 len);
    void HandleQueryUser(uint8 * buf, uint16 len);
    void HandleRecharge(uint8 * buf, uint16 len);
    void HandleGetTime(uint8 * buf, uint16 len);

protected:
    int com;

private:
    static CCardHost * instance;
    cbuffer_t cmdbuf;
};
#endif
