#ifndef _CMAINLOOP_H
#define _CMAINLOOP_H
#include "sysdefs.h"

class CMainLoop {
public:
    static CMainLoop * GetInstance();
    void Run();
    void SetCom(int com) {this->com = com;};
    void SetValveCount(int count) {this->valveCount = count;};
private:
    CMainLoop();
    ~CMainLoop();
    void Draw();
    void DrawInitial();
    void DrawStatus();
    void DrawGPRS();
    void DrawForwarder();
    void DrawValve();
    void DrawHeat();
    void DrawCardHost();
    void DrawUsbPlugin();
    void DrawUsbBackup();
    void DrawUsbDone();
    void Write(uint8 * data, uint16 len);
    static CMainLoop * instance;
    int kfd;
    uint8 hidx, vidx; // indexes for screen
    uint8 cardidx;
    uint8 * borders;
    bool usbmode;
    int usbstate;
    int com;
    int valveCount;
    bool checkValve;
};
#endif
