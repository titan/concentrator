#ifndef __CCARDHOST_H
#define __CCARDHOST_H
#include "IThread.h"
#include "cbuffer.h"
#include "CLock.h"
#include <vector>
#include "sysdefs.h"
using namespace std;

typedef struct {
    uint8 x[6];
} cardaddr_t;

class CCardHost: public IThread
{

public:
    static CCardHost * GetInstance();
    void SetCom(int com) {
        this->com = com;
    };
    void AckQueryUser(userid_t uid, uint8 * data, uint16 len); // called by IValveMonitor
    void AckRecharge(userid_t uid, uint8 * data, uint16 len); // called by IValveMonitor
    void AckTimeOrRemove(uint8 * data, uint16 len);
    bool GetCardInfo(vector<cardaddr_t> &);
    void SetGPIO(gpio_name_t gpio) {
        this->gpio = gpio;
    };

private:
    CCardHost();
    ~CCardHost();
    virtual uint32 Run();
    void ParseAndExecute(uint8 * cmd, uint16 len);
    void HandleQueryUser(uint8 * buf, uint16 len);
    void HandleRecharge(uint8 * buf, uint16 len);
    void HandleGetTime(uint8 * buf, uint16 len);
    void HandleInfo(uint8 * buf, uint16 len);

protected:

private:
    int com;
    static CCardHost * instance;
    cbuffer_t cmdbuf;
    vector<cardaddr_t> info;
    bool infoGotten;
    gpio_name_t gpio;
};
#endif
