#include "../renderer.hpp"
#include "../filesystem.hpp"
#include "../constants.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <assert.h>

namespace ENGINE::GENERIC {
        
    void GLShader::init(const char *path, GLenum type) {
        ENGINE::File *f = ENGINE::g_fileSystemInstance.get()->findFile(path);
        uint32_t fsize = f->getSize();

        char* src = new char[fsize + 1]; // +1 for null terminator
        f->read(src, fsize);
        src[fsize] = '\0';

        const char* ptr = src;

        id = glCreateShader(type);
        glShaderSource(id, 1, &ptr, NULL);
        glCompileShader(id);

        int success;
        glGetShaderiv(id, GL_COMPILE_STATUS, &success);
        assert(success);
        delete[] src; // clean up
        
    }

    GLRenderer::GLRenderer(void) {

        // Initialize SDL video subsystem
        assert(SDL_Init(SDL_INIT_VIDEO) == 0);

        // Set OpenGL version (3.3 Core)
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

#ifdef __APPLE__
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
#endif

        scrw = ENGINE::CONST::SCREEN_WIDTH;
        scrh = ENGINE::CONST::SCREEN_HEIGHT;

        // Create SDL window with OpenGL
        window = SDL_CreateWindow(
            "titletemp",
            SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,
            scrw,
            scrh,
            SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
        );

        assert(window);

        SDL_GLContext glContext = SDL_GL_CreateContext(window);
        assert(glContext);

        int err;
        err = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
        assert(err);

        // link shaders
        vertshader.init("assets/vertex.glsl", GL_VERTEX_SHADER);
        fragshader.init("assets/fragment.glsl", GL_FRAGMENT_SHADER);
        shaderprog = glCreateProgram();
        glAttachShader(shaderprog, vertshader.getId());
        glAttachShader(shaderprog, fragshader.getId());
        glLinkProgram(shaderprog);
        // check for linking errors
        glGetProgramiv(shaderprog, GL_LINK_STATUS, &err);
        assert(err);
        vertshader.free();
        fragshader.free();

        glViewport(0, 0, scrw, scrh);
        setClearCol(64, 64, 64);
    }

    void GLRenderer::beginFrame(void) {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void GLRenderer::endFrame(void) {
        SDL_GL_SwapWindow(window);
    }
        
    void GLRenderer::drawTri(const ENGINE::COMMON::TRI32 &tri, uint32_t z, uint32_t col) {
        float vertices[] = {
            (float)tri.pos[0].x, (float)tri.pos[0].y,
            (float)tri.pos[1].x, (float)tri.pos[1].y,
            (float)tri.pos[2].x, (float)tri.pos[2].y
        };

        GLuint vao, vbo;
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glUseProgram(shaderprog);

        GLint screenLoc = glGetUniformLocation(shaderprog, "uScreenSize");
        glUniform2f(screenLoc, (float)scrw, (float)scrh);

        float r = ((col >> 24) & 0xFF) / 255.0f;
        float g = ((col >> 16) & 0xFF) / 255.0f;
        float b = ((col >> 8) & 0xFF) / 255.0f;
        float a = (col & 0xFF) / 255.0f;

        GLint colorLoc = glGetUniformLocation(shaderprog, "uColor");
        glUniform4f(colorLoc, r, g, b, a);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);

        glDrawArrays(GL_TRIANGLES, 0, 3);

        glDeleteBuffers(1, &vbo);
        glDeleteVertexArrays(1, &vao);
    }

    void GLRenderer::drawRect(const ENGINE::COMMON::RECT32 &rect, uint32_t z, uint32_t col) {
        ENGINE::COMMON::XY32 p0(rect.x, rect.y);
        ENGINE::COMMON::XY32 p1(rect.x + rect.w, rect.y);
        ENGINE::COMMON::XY32 p2(rect.x + rect.w, rect.y + rect.h);
        ENGINE::COMMON::XY32 p3(rect.x, rect.y + rect.h);

        ENGINE::COMMON::TRI32 t1(p0, p1, p2);
        ENGINE::COMMON::TRI32 t2(p0, p2, p3);

        drawTri(t1, z, col);
        drawTri(t2, z, col);
    }

	//void GLRenderer::drawTexRect(const TextureInfo &tex, ENGINE::COMMON::XY32 &pos, uint32_t z, uint32_t col) {}
	//void GLRenderer::drawTexQuad(const TextureInfo &tex, ENGINE::COMMON::RECT32 &pos, uint32_t z, uint32_t col) {}
	//void GLRenderer::drawModel(const Model *model, const FIXED::Vector12 &pos, FIXED::Vector12 rot) {}

} //namespace ENGINE::GENERIC 