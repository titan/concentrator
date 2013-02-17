#ifndef __CSCREEN_H
#define __CSCREEN_H
#include"CLock.h"
#include"CTimer.h"
#include"CHeatMonitor.h"
#include"CForwarderMonitor.h"
enum ScreenE
{
   SCREEN_POWERON,
   SCREEN_MACHINE_STATUS,
   SCREEN_GPRS_STATUS,
   SCREEN_FORWARDER_INFO,
   SCREEN_GENERALHEAT_INFO//keep this at the last position
};
enum KeyE
{
   KEY_LEFT,
   KEY_RIGHT,
   KEY_UP,
   KEY_DOWN,
   KEY_ENTER,
   KEY_ESC,
   KEY_MAX,//never delete this
};
enum ScreenLineE
{
   SCREEN_LINE_1_1,
   SCREEN_LINE_2_1,
   SCREEN_LINE_2_2,
   SCREEN_LINE_3_1,
   SCREEN_LINE_3_2,
   SCREEN_LINE_3_3,
   SCREEN_LINE_MAX//never delete this
};
class CScreen
{
   public:
      static CScreen* GetInstance();
   private:
      CScreen():m_CurrentScreen(SCREEN_POWERON), m_SubScreen(0), m_IsFirstLineDataReady(false), m_IsSecondLineDataReady(false), m_IsThirdLineDataReady(false){
          m_IsFirstLineDrawn = false;
          m_IsSecondLineDrawn = false;
          m_IsThirdLineDrawn = false;
      };
      static CScreen* m_Instance;
      static CLock m_ScreenLock;
      uint32 m_CurrentScreen;
      uint32 m_SubScreen;
      CTimer m_Timer;
   public:
      void PowerOn();
      void InputKey(KeyE KeyValue);
      void Draw();

   private:
      inline void ClearScreen();
      void ClearScreenLineRect(ScreenLineE nLineIndex);
   private:
      void DrawPowerOnInit();
      void DrawMachineStatus();
      void DrawGPRSStatus();
      void DrawWarnings();
      void DrawErrors();
      void DrawShareINFO();
      void DrawForwarderINFO();
      void DrawGeneralHeatINFO();

   private:
      bool IsDataInvalid();
      void ClearLineData();
      void ClearLineDraw();
   private:
      bool m_IsFirstLineDataReady;
      bool m_IsFirstLineDrawn;
      bool m_IsSecondLineDataReady;
      bool m_IsSecondLineDrawn;
      bool m_IsThirdLineDataReady;
      bool m_IsThirdLineDrawn;
};
#endif
