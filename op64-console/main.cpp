#include "emulator.h"
#include "logger.h"
#include <cstdio>

//#define GLEW_STATIC
#include <GL/glew.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#define GLFW_EXPOSE_NATIVE_WGL
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

void printDebugMsg(const char* msg)
{
    printf(msg);
}

int main(int argc, char *argv[])
{
    LOG.setLogCallback(printDebugMsg);

    glfwInit();

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);

    glfwWindowHint(GLFW_RESIZABLE, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(320, 240, "OpenGL", nullptr, nullptr);
    //glfwMakeContextCurrent(window);

    glewExperimental = GL_TRUE;
    glewInit();

    if (EMU.loadRom(argv[1]))
    {
        EMU.registerRenderWindow(glfwGetWin32Window(window));
        if (EMU.initializeHardware())
        {
            EMU.execute();

            EMU.uninitializeHardware();
        }
    }

    glfwTerminate();
}

