// Cross-platform entry point for ApartmentSimulator
#include "platform.h"

#if PLATFORM_WINDOWS
    #include <Windows.h>
    #include "../include/ApartmentSimulator.h"

    int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
        (void)nCmdShow;
        (void)lpCmdLine;
        (void)hPrevInstance;

        YDS_ERROR_DECLARE("WinMain");

        ApartmentSimulator apartmentSimulator;
        apartmentSimulator.Initialize(ysDevice::DeviceAPI::DirectX11, (void *)&hInstance);
        apartmentSimulator.GameLoop();
        apartmentSimulator.Destroy();

        YDS_ERROR_RETURN_STATIC(ysError::None);
        return 0;
    }

#else
    // Linux / macOS: use SDL2 backend
    #include <SDL2/SDL.h>
    #include "../include/ApartmentSimulator.h"

    int main(int argc, char *argv[]) {
        (void)argc;
        (void)argv;

        YDS_ERROR_DECLARE("main");

        ApartmentSimulator apartmentSimulator;
        // Pass nullptr as instance; the SDL backend ignores it
        apartmentSimulator.Initialize(ysDevice::DeviceAPI::OpenGL4_0, nullptr);
        apartmentSimulator.GameLoop();
        apartmentSimulator.Destroy();

        YDS_ERROR_RETURN_STATIC(ysError::None);
        return 0;
    }
#endif