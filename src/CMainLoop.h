#ifndef _CMAINLOOP_H
#define _CMAINLOOP_H
#include "sysdefs.h"

class CMainLoop {
public:
    static CMainLoop * GetInstance();
    void Run();
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
    static CMainLoop * instance;
    int kfd;
    uint8 hidx, vidx; // indexes for screen
    uint8 cardidx;
    uint8 * borders;
    bool usbmode;
};
#endif
