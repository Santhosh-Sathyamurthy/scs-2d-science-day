#ifndef OPENGL_H
#define OPENGL_H

#include "platform.h"

#if PLATFORM_WINDOWS
    #include <Windows.h>
    #include <gl/gl.h>
    #include <gl/glext.h>
    #include <gl/wglext.h>
#elif PLATFORM_MACOS
    // macOS ships OpenGL in a framework
    #define GL_SILENCE_DEPRECATION
    #include <OpenGL/gl3.h>
    #include <OpenGL/gl3ext.h>
#else
    // Linux / WSL2 — use system GLEW or glad
    #define GL_GLEXT_PROTOTYPES
    #include <GL/gl.h>
    #include <GL/glext.h>
    // WGL is Windows-only; on Linux we use GLX (included via SDL2 / GLFW context)
    // so wglext.h is simply not needed here.
#endif

#endif /* OPENGL_H */