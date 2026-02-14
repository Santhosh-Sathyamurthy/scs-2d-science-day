#ifndef YDS_WINDOWS_INPUT_SYSTEM_H
#define YDS_WINDOWS_INPUT_SYSTEM_H

#include "platform.h"
#include "yds_input_system.h"

#include <vector>

// Forward declare platform types so the header compiles everywhere
#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
#else
    // Stub types — only used in the Windows-specific .cpp implementation
    typedef void *HRAWINPUT;
    typedef long  LPARAM;
    typedef unsigned long long WPARAM;
#endif

class ysWindowsInputDevice;

class ysWindowsInputSystem : public ysInputSystem {
public:
    ysWindowsInputSystem();
    virtual ~ysWindowsInputSystem();

#if PLATFORM_WINDOWS
    int ProcessInputMessage(HRAWINPUT lparam);
    int OnOsKey(LPARAM lParam, WPARAM wParam);
    int OnOsMouseButtonDown(ysMouse::Button button);
    int OnOsMouseButtonUp(ysMouse::Button button);
    int OnOsMouseWheel(LPARAM lParam, WPARAM wParam);
    int OnOsMouseMove(LPARAM lParam, WPARAM wParam);
#else
    // Stubs for non-Windows builds — not called at runtime
    int ProcessInputMessage(void *lparam);
    int OnOsKey(long lParam, unsigned long long wParam);
    int OnOsMouseButtonDown(ysMouse::Button button);
    int OnOsMouseButtonUp(ysMouse::Button button);
    int OnOsMouseWheel(long lParam, unsigned long long wParam);
    int OnOsMouseMove(long lParam, unsigned long long wParam);
#endif

protected:
    virtual ysError CreateDevices() override;
    virtual ysError CheckDeviceStatus(ysInputDevice *device) override;
    virtual ysError CheckAllDevices() override;

    virtual ysInputDevice *CreateDevice(ysInputDevice::InputDeviceType type, int id) override;
    virtual ysInputDevice *CreateVirtualDevice(ysInputDevice::InputDeviceType type) override;

#if PLATFORM_WINDOWS
    ysWindowsInputDevice *AddDevice(RAWINPUT *rawInput);
    ysWindowsInputDevice *DeviceLookup(HANDLE hDevice);
    ysWindowsInputDevice *SystemNameDeviceLookup(wchar_t *systemName);
    static ysInputDevice::InputDeviceType TranslateType(int type);
#else
    ysWindowsInputDevice *AddDevice(void *rawInput);
    ysWindowsInputDevice *DeviceLookup(void *hDevice);
    ysWindowsInputDevice *SystemNameDeviceLookup(wchar_t *systemName);
    static ysInputDevice::InputDeviceType TranslateType(int type);
#endif

private:
    std::vector<unsigned char> m_rawInputBuffer;
};

#endif /* YDS_WINDOWS_INPUT_SYSTEM_H */