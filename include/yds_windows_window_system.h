#ifndef YDS_WINDOWS_WINDOW_SYSTEM_H
#define YDS_WINDOWS_WINDOW_SYSTEM_H

#include "platform.h"
#include "yds_window_system.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
#else
    // Minimal stubs so the class declaration compiles on Linux/macOS.
    // The implementation file guards its body with #if PLATFORM_WINDOWS.
    typedef void   *HINSTANCE;
    typedef void   *HWND;
    typedef void   *HCURSOR;
    typedef struct { long left, top, right, bottom; } RECT;
    typedef unsigned int  UINT;
    typedef long          LPARAM;
    typedef unsigned long long WPARAM;
    typedef long          LRESULT;
#endif

class ysWindowsWindowSystem : public ysWindowSystem {
public:
    ysWindowsWindowSystem();
    ~ysWindowsWindowSystem();

    virtual ysError NewWindow(ysWindow **newWindow) override;
    virtual ysMonitor *NewMonitor() override;
    virtual ysMonitor *MonitorFromWindow(ysWindow *window) override;
    virtual ysError SurveyMonitors() override;
    virtual void ProcessMessages() override;

    ysWindow *FindWindowFromHandle(HWND handle);

    virtual void ConnectInstance(void *genericInstanceConnection) override;

#if PLATFORM_WINDOWS
    static LRESULT WINAPI WinProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

    virtual void ConfineCursor(ysWindow *window) override;
    virtual void ReleaseCursor() override;
    virtual void SetCursorPosition(int x, int y) override;
    virtual void SetCursorVisible(bool visible) override;
    virtual void SetCursor(Cursor cursor) override;

    ysMonitor *FindMonitorFromHandle(HMONITOR handle);

protected:
    HINSTANCE m_instance;
    RECT      m_oldCursorClip;
    HCURSOR   m_oldCursor;
};

#endif /* YDS_WINDOWS_WINDOW_SYSTEM_H */