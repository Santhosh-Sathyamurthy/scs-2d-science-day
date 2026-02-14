#ifndef YDS_VULKAN_H
#define YDS_VULKAN_H

#include "platform.h"

#define DELTA_ENABLE_VULKAN 0

#if DELTA_ENABLE_VULKAN
    #include <vulkan/vulkan_core.h>

    #if PLATFORM_WINDOWS
        #define NOMINMAX
        #include <Windows.h>
        #include <vulkan/vulkan_win32.h>
    #elif PLATFORM_LINUX
        // Define VK_USE_PLATFORM_XCB_KHR or XLIB_KHR before including
        // depending on your display server. SDL2 handles this automatically.
        // #define VK_USE_PLATFORM_XCB_KHR
        // #include <vulkan/vulkan_xcb.h>
    #elif PLATFORM_MACOS
        // #define VK_USE_PLATFORM_METAL_EXT
        // #include <vulkan/vulkan_metal.h>
    #endif
#endif /* DELTA_ENABLE_VULKAN */

#endif /* YDS_VULKAN_H */