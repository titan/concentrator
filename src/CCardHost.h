#ifndef __CCARDHOST_H
#define __CCARDHOST_H
#include "IThread.h"
#include "CSerialComm.h"
#include "cbuffer.h"

class CCardHost:public IThread {

public:
    static CCardHost * GetInstance();
    void SetCom(int com) {this->com = com;};

private:
    CCardHost();
    ~CCardHost();
    virtual uint32 Run();
    void ParseAndExecute(uint8 * cmd, uint16 len);
    void AckGetTime();

protected:
    int com;

private:
    static CCardHost * instance;
    cbuffer_t cmdbuf;
};
#endif
