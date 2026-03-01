#ifndef YDS_AUDIO_DEVICE_H
#define YDS_AUDIO_DEVICE_H

#include "yds_audio_system_object.h"
#include "yds_audio_parameters.h"

#include "yds_window.h"

#include <cstring>
#include <cwchar>

class ysAudioSource;
class ysAudioBuffer;

class ysAudioDevice : public ysAudioSystemObject {
    friend class ysAudioSystem;

public:
    static constexpr int MaxDeviceNameLength = 256;

public:
    ysAudioDevice();
    ysAudioDevice(API api);
    ~ysAudioDevice();

    bool IsConnected() const { return m_connected; }
    void SetDeviceName(const wchar_t *newName) { 
        #if defined(_WIN32) || defined(_WIN64)
            wcscpy_s(m_deviceName, MaxDeviceNameLength, newName);
        #else
            wcsncpy(m_deviceName, newName, MaxDeviceNameLength - 1);
            m_deviceName[MaxDeviceNameLength - 1] = L'\0';
        #endif
    }
    const wchar_t *GetDeviceName() const { return m_deviceName; }

    virtual ysError CreateBuffer(const ysAudioParameters *parameters, SampleOffset size, ysAudioBuffer **buffer) = 0;
    virtual ysError CreateSource(const ysAudioParameters *parameters, SampleOffset size, ysAudioSource **source) = 0;
    virtual ysError CreateSource(ysAudioBuffer *sourceBuffer, ysAudioSource **source) = 0;

    virtual void UpdateAudioSources() = 0;

    virtual ysError DestroyAudioBuffer(ysAudioBuffer *&buffer);
    virtual ysError DestroyAudioSource(ysAudioSource *&source);
    ysError DestroyAudioBuffers();

protected:
    wchar_t m_deviceName[MaxDeviceNameLength];
    ysWindow *m_windowAssociation;

    ysDynamicArray<ysAudioBuffer, 4> m_audioBuffers;
    ysDynamicArray<ysAudioSource, 4> m_audioSources;

    bool m_connected;
};

#endif /* YDS_AUDIO_DEVICE_H */