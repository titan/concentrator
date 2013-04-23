#ifndef _IVALVE_MONITOR_FACTORY
#define _IVALVE_MONITOR_FACTORY

#include "CValveMonitor.h"
#include "CForwarderMonitor.h"

extern bool wireless;

class IValveMonitorFactory {
public:
    static IValveMonitor * GetInstance() {
        if (wireless) {
            return CForwarderMonitor::GetInstance();
        } else {
            return CValveMonitor::GetInstance();
        }
    };
};
#endif
