// Cross-platform entry point for delta-basic-demo
#include "platform.h"

#if PLATFORM_WINDOWS
    #define NOMINMAX
    #include <Windows.h>
    #include "../include/demo_list.h"

    int WINAPI WinMain(
        _In_ HINSTANCE hInstance,
        _In_opt_ HINSTANCE hPrevInstance,
        _In_ LPSTR lpCmdLine,
        _In_ int nCmdShow)
    {
        (void)nCmdShow;
        (void)lpCmdLine;
        (void)hPrevInstance;

        ysContextObject::DeviceAPI api = ysContextObject::DeviceAPI::DirectX11;
        delta_demo::DemoApplication::Demo demo =
            delta_demo::DemoApplication::Demo::DeltaBasicDemo;

        delta_demo::DemoApplication *app = delta_demo::CreateApplication(demo);
        app->Initialize((void *)&hInstance, api);
        app->Run();
        app->Destroy();

        return 0;
    }

#else
    #include <SDL2/SDL.h>
    #include "../include/demo_list.h"

    int main(int argc, char *argv[]) {
        (void)argc;
        (void)argv;

        // Use OpenGL on Linux/macOS
        ysContextObject::DeviceAPI api = ysContextObject::DeviceAPI::OpenGL4_0;
        delta_demo::DemoApplication::Demo demo =
            delta_demo::DemoApplication::Demo::DeltaBasicDemo;

        delta_demo::DemoApplication *app = delta_demo::CreateApplication(demo);
        app->Initialize(nullptr, api);
        app->Run();
        app->Destroy();

        return 0;
    }
#endif