#include "../include/os_utilities.h"
#include "../include/path.h"
#include "platform.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>

    dbasic::Path dbasic::GetModulePath() {
        wchar_t path[MAX_PATH];
        GetModuleFileName(NULL, path, MAX_PATH);

        Path fullPath = Path(path);
        Path parentPath;
        fullPath.GetParentPath(&parentPath);
        return parentPath;
    }

#else
    // Linux / macOS implementation
    #include <unistd.h>
    #include <limits.h>
    #include <string>

    #if PLATFORM_MACOS
        #include <mach-o/dyld.h>
    #endif

    dbasic::Path dbasic::GetModulePath() {
        char buf[PATH_MAX] = {0};

    #if PLATFORM_MACOS
        uint32_t size = sizeof(buf);
        _NSGetExecutablePath(buf, &size);
    #else
        // Linux: read /proc/self/exe
        ssize_t len = readlink("/proc/self/exe", buf, sizeof(buf) - 1);
        if (len == -1) buf[0] = '\0';
        else           buf[len] = '\0';
    #endif

        // Convert narrow string to wide for Path (if Path takes wchar_t)
        wchar_t wbuf[PATH_MAX] = {0};
        mbstowcs(wbuf, buf, PATH_MAX);

        Path fullPath = Path(wbuf);
        Path parentPath;
        fullPath.GetParentPath(&parentPath);
        return parentPath;
    }
#endif