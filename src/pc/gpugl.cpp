#include "gpugl.hpp"
#include <assert.h>
#include <cassert>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

namespace GFX {

//todo temp
const char* vertexShaderSrc = R"(
#version 330 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
out vec3 vColor;
void main() {
    vColor = color;
    gl_Position = vec4(position, 1.0);
}
)";

const char* fragmentShaderSrc = R"(
#version 330 core
in vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    assert(success);
    return shader;
}

void GLRenderer::init(int mode) {
    assert(glfwInit());
#ifdef __APPLE__
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "OpenGL Renderer", nullptr, nullptr);
    assert(window);

    glfwMakeContextCurrent(window);
    assert(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress));

    GLuint vs = compileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint success;
    glGetProgramiv(prog, GL_LINK_STATUS, &success);
    assert(success);

    glDeleteShader(vs);
    glDeleteShader(fs);

    framecounter = 0;
    setClearCol(64, 64, 64);
}

void GLRenderer::beginFrame(void) {
    GLuint r = (clearcol >> 16) & 0xFF;
    GLuint g = (clearcol >> 8) & 0xFF;
    GLuint b = clearcol & 0xFF;
    GLuint a = 0xFF; // opaque

    printf("%d %d %d\n", r, g, b);

    glClearColor(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GLRenderer::endFrame(void) {
    framecounter++;
    glUseProgram(prog);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwSwapBuffers(window);
    glfwPollEvents();
}

void GLRenderer::drawRect(RECT<int32_t> rect, int z, uint32_t col) {
    float fr = ((col >> 0) & 0xFF) / 255.0f;
    float fg = ((col >> 8) & 0xFF) / 255.0f;
    float fb = ((col >> 16) & 0xFF) / 255.0f;
    //normalize to screen coordinates
    float x0 = (float)rect.x / (SCREEN_WIDTH / 2) - 1.0f;
    float y0 = 1.0f - (float)rect.y / (SCREEN_HEIGHT / 2);
    float x1 = ((float)rect.x + rect.w) / (SCREEN_WIDTH / 2) - 1.0f;
    float y1 = 1.0f - ((float)rect.y + rect.h) / (SCREEN_HEIGHT / 2);
    //rectangle verts
    float vertices[] = {
        x0, y0, 0.0f, fr, fg, fb,  // top-left
        x0, y1, 0.0f, fr, fg, fb,  // bottom-left
        x1, y1, 0.0f, fr, fg, fb,  // bottom-right

        x0, y0, 0.0f, fr, fg, fb,  // top-left
        x1, y1, 0.0f, fr, fg, fb,  // bottom-right
        x1, y0, 0.0f, fr, fg, fb   // top-right
    };

    GLuint tempVao, tempVbo;
    glGenVertexArrays(1, &tempVao);
    glGenBuffers(1, &tempVbo);

    glBindVertexArray(tempVao);
    glBindBuffer(GL_ARRAY_BUFFER, tempVbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6*sizeof(float), (void*)(3*sizeof(float)));
    glEnableVertexAttribArray(1);

    glUseProgram(prog);
    glBindVertexArray(tempVao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDeleteBuffers(1, &tempVbo);
    glDeleteVertexArrays(1, &tempVao);
}

void GLRenderer::drawTexRect(const TextureInfo &tex, XY<int32_t> pos, int z, int col) {

}

void GLRenderer::drawTexQuad(const TextureInfo &tex, RECT<int32_t> pos, int z, uint32_t col) {
 
}

void GLRenderer::drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {
	
}

void GLRenderer::printString(XY<int32_t> pos, int z, const char *str) {
	assert(fontmap);
}

void GLRenderer::printStringf(XY<int32_t> pos, int z, const char *fmt, ...) {
    char buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    printString(pos, z, buf);
}
}