#include "../include/yds_timing.h"
#include "platform.h"

// ── Platform-specific time / clock headers ──────────────────────────────────
#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
    #include <mmsystem.h>
    #include <intrin.h>

    static bool        s_qpcFlag;
    static LARGE_INTEGER s_qpcFrequency;

    static uint64_t PlatformTime() {
        if (s_qpcFlag) {
            LARGE_INTEGER cur;
            QueryPerformanceCounter(&cur);
            return uint64_t(cur.QuadPart);
        }
        return uint64_t(timeGetTime()) * 1000;
    }

    static void PlatformTimeInit() {
        s_qpcFlag = (QueryPerformanceFrequency(&s_qpcFrequency) > 0);
    }

    static uint64_t PlatformFrequency() {
        return s_qpcFlag ? uint64_t(s_qpcFrequency.QuadPart) : 1000ULL;
    }

    static uint64_t PlatformClock() {
        return uint64_t(__rdtsc());
    }

#else
    // Linux / macOS — use POSIX clock_gettime
    #include <time.h>
    #include <stdint.h>

    // Fixed frequency: we report time in microseconds directly.
    static constexpr uint64_t POSIX_FREQ = 1'000'000ULL; // µs

    static uint64_t PlatformTime() {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        return uint64_t(ts.tv_sec) * 1'000'000ULL +
               uint64_t(ts.tv_nsec) / 1000ULL;
    }

    static void PlatformTimeInit() { /* nothing needed */ }

    static uint64_t PlatformFrequency() { return POSIX_FREQ; }

    // Best available cycle counter on x86-64 Linux
    #if defined(__x86_64__) || defined(__i386__)
        #include <x86intrin.h>
        static uint64_t PlatformClock() { return __rdtsc(); }
    #else
        // ARM / other: fall back to CLOCK_MONOTONIC_RAW ns
        static uint64_t PlatformClock() {
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
            return uint64_t(ts.tv_sec) * 1'000'000'000ULL + uint64_t(ts.tv_nsec);
        }
    #endif
#endif
// ────────────────────────────────────────────────────────────────────────────

ysTimingSystem *ysTimingSystem::g_instance = nullptr;

uint64_t ysTimingSystem::GetTime()  { return PlatformTime();  }
unsigned __int64 ysTimingSystem::GetClock() { return PlatformClock(); }

ysTimingSystem::ysTimingSystem() {
    m_frameDurations = nullptr;
    m_durationSamples = 0;
    SetPrecisionMode(Precision::Microsecond);
    Initialize();
}

ysTimingSystem::~ysTimingSystem() { /* void */ }

void ysTimingSystem::SetPrecisionMode(Precision mode) {
    m_precisionMode = mode;
    if      (mode == Precision::Millisecond) m_div = 1000.0;
    else if (mode == Precision::Microsecond) m_div = 1000000.0;
}

double ysTimingSystem::ConvertToSeconds(uint64_t t_u) { return t_u / m_div; }

void ysTimingSystem::Initialize() {
    PlatformTimeInit();

    m_frameNumber = 0;
    m_lastFrameTimestamp = GetTime();
    m_lastFrameDuration  = 0;
    m_lastFrameClockstamp = GetClock();
    m_lastFrameClockTicks = 0;
    m_isPaused = false;
    m_averageFrameDuration = 0;
    m_fps = 1024.0;

    m_frameDurations = new double[DurationSamples];
    m_durationSamples = 0;
    m_durationSampleWriteIndex = 0;
}

void ysTimingSystem::Update() {
    if (!m_isPaused) { m_frameNumber++; }

    const uint64_t thisTime = GetTime();
    // Convert raw ticks to microseconds using the platform frequency
    m_lastFrameDuration = ((thisTime - m_lastFrameTimestamp) * 1'000'000ULL)
                          / PlatformFrequency();
    m_lastFrameTimestamp = thisTime;

    const uint64_t thisClock = GetClock();
    m_lastFrameClockTicks  = thisClock - m_lastFrameClockstamp;
    m_lastFrameClockstamp  = thisClock;

    if (m_frameNumber > 1) {
        m_frameDurations[m_durationSampleWriteIndex] =
                double(m_lastFrameDuration) / m_div;
        if (++m_durationSampleWriteIndex >= DurationSamples)
            m_durationSampleWriteIndex = 0;

        if (m_durationSamples < DurationSamples) ++m_durationSamples;

        m_averageFrameDuration = 0;
        for (int i = 0; i < m_durationSamples; ++i)
            m_averageFrameDuration += m_frameDurations[i];

        if (m_durationSamples > 0)
            m_averageFrameDuration /= m_durationSamples;

        if (m_averageFrameDuration != 0)
            m_fps = 1.0f / float(m_averageFrameDuration);
    }
}

void ysTimingSystem::RestartFrame() {
    m_lastFrameTimestamp = GetTime();
}

double   ysTimingSystem::GetFrameDuration()    { return m_lastFrameDuration / m_div; }
uint64_t ysTimingSystem::GetFrameDuration_us() { return m_lastFrameDuration; }