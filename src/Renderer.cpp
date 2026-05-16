#include "Renderer.h"

void GLAPIENTRY MessageCallback(GLenum source, GLenum type, GLuint id, GLenum severity,
                                GLsizei length, const GLchar *message, const void *userParam)
{
#ifdef GL_DEBUG_OUTPUT
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION || severity == GL_DEBUG_SEVERITY_LOW || severity == GL_DEBUG_SEVERITY_MEDIUM)
        return;

    std::cerr << "OpenGL ";
    if (severity == GL_DEBUG_SEVERITY_HIGH)
        std::cerr << "ERROR";
    else
        std::cerr << "WARNING";
    std::cerr << " (" << id << "): " << message << std::endl;
#endif
}

void enableDebugging()
{
#ifdef GL_DEBUG_OUTPUT
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(MessageCallback, nullptr);
    glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_NOTIFICATION, 0, nullptr, GL_FALSE);
#endif
}
