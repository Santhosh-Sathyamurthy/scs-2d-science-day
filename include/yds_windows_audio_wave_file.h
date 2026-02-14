#ifndef YDS_WINDOWS_AUDIO_WAVE_FILE_H
#define YDS_WINDOWS_AUDIO_WAVE_FILE_H

#include "platform.h"
#include "yds_audio_file.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
    // HMMIO and waveform audio types
    #include <mmsystem.h>
#else
    // Portable substitute: use a plain FILE* and parse the WAV header ourselves
    #include <stdio.h>
    typedef FILE *HMMIO; // repurposed to FILE* on Linux/macOS
#endif

class ysWindowsAudioWaveFile : public ysAudioFile {
public:
    ysWindowsAudioWaveFile();
    ~ysWindowsAudioWaveFile();

    virtual Error OpenFile(const wchar_t *fname) override;
    virtual Error CloseFile() override;

protected:
    virtual Error GenericRead(SampleOffset offset, SampleOffset size, void *buffer) override;

    HMMIO        m_fileHandle;
    unsigned int m_dataSegmentOffset;
};

#endif /* YDS_WINDOWS_AUDIO_WAVE_FILE_H */