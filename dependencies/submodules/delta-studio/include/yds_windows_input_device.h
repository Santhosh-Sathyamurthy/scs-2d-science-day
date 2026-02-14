#ifndef YDS_WINDOWS_INPUT_DEVICE_H
#define YDS_WINDOWS_INPUT_DEVICE_H

#include "platform.h"
#include "yds_input_device.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
#else
    // Stub Windows types for non-Windows builds
    typedef void   *HANDLE;
    struct RID_DEVICE_INFO { unsigned int dwType; /* minimal stub */ };
#endif

class ysWindowsInputDevice : public ysInputDevice {
    friend class ysWindowsInputSystem;

public:
    ysWindowsInputDevice();
    ysWindowsInputDevice(InputDeviceType type);
    ~ysWindowsInputDevice();

    void MakeGeneric();

protected:
    HANDLE         m_deviceHandle;
    RID_DEVICE_INFO m_info;
    wchar_t        m_systemName[256];
};

#endif /* YDS_WINDOWS_INPUT_DEVICE_H */