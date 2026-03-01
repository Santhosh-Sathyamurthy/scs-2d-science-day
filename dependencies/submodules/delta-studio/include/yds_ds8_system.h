#ifndef YDS_DS8_SYSTEM_H
#define YDS_DS8_SYSTEM_H

#include "yds_audio_system.h"
#include "yds_window.h"

// DirectSound is Windows-only - guard all DirectSound-specific code
#if defined(_WIN32) || defined(_WIN64)
    #include <dsound.h>
    #define YDS_DIRECTSOUND_AVAILABLE 1
#else
    #define YDS_DIRECTSOUND_AVAILABLE 0
    // Provide stub types for non-Windows platforms to allow compilation
    typedef void* LPGUID;
    typedef const char* LPCTSTR;
    typedef void* LPVOID;
    typedef int BOOL;
    #ifndef CALLBACK
        #define CALLBACK
    #endif
    
    // Stub GUID structure for non-Windows
    struct GUID {
        unsigned long  Data1;
        unsigned short Data2;
        unsigned short Data3;
        unsigned char  Data4[8];
    };
#endif

class ysDS8Device;

class ysDS8System : public ysAudioSystem {
public:
    ysDS8System();
    virtual ~ysDS8System();

    virtual ysError EnumerateDevices() override;
    virtual ysError ConnectDevice(ysAudioDevice *device, ysWindow *windowAssociation) override;
    virtual ysError ConnectDeviceConsole(ysAudioDevice *device) override;
    virtual ysError DisconnectDevice(ysAudioDevice *device) override;

protected:
    ysDS8Device *AddDS8Device();

    static BOOL CALLBACK DirectSoundEnumProc(LPGUID lpGUID, LPCTSTR lpszDesc, LPCTSTR lpszDrvName, LPVOID lpContext);
    GUID m_primaryDeviceGuid;
};

#endif /* YDS_DS8_SYSTEM_H */