#ifndef YDS_WINDOWS_MONITOR_H
#define YDS_WINDOWS_MONITOR_H

#include "platform.h"
#include "yds_monitor.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
#else
    // Stub type — HMONITOR is just a pointer on Windows
    typedef void *HMONITOR;
#endif

class ysWindowsMonitor : public ysMonitor {
public:
    ysWindowsMonitor();
    ~ysWindowsMonitor();

    void Initialize(HMONITOR monitor) { m_handle = monitor; }
    HMONITOR Handle() const           { return m_handle; }

private:
    HMONITOR m_handle;
};

#endif /* YDS_WINDOWS_MONITOR_H */