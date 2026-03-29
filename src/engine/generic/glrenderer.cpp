#include "../renderer.hpp"
#include "../filesystem.hpp"
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

        scrw = ENGINE::COMMON::SCREEN_WIDTH;
        scrh = ENGINE::COMMON::SCREEN_HEIGHT;

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
        vertshader.init("", GL_VERTEX_SHADER);
        fragshader.init("", GL_FRAGMENT_SHADER);
        shaderprog = glCreateProgram();
        glAttachShader(shaderprog, vertshader.getId());
        glAttachShader(shaderprog, fragshader.getId());
        glLinkProgram(shaderprog);
        // check for linking errors
        glGetProgramiv(shaderprog, GL_LINK_STATUS, &err);
        assert(err);
        vertshader.free();
        fragshader.free();

        setClearCol(64, 64, 64);
    }

    void GLRenderer::beginFrame(void) {
        glClear(GL_COLOR_BUFFER_BIT);
    }

    void GLRenderer::endFrame(void) {
        SDL_GL_SwapWindow(window);
    }

    void GLRenderer::drawRect(ENGINE::COMMON::RECT32 rect, int z, uint32_t col) {
    }

	//void GLRenderer::drawTexRect(const TextureInfo &tex, ENGINE::COMMON::XY<int32_t> pos, int z, int col) {}
	//void GLRenderer::drawTexQuad(const TextureInfo &tex, ENGINE::COMMON::RECT<int32_t> pos, int z, uint32_t col) {}
	//void GLRenderer::drawModel(const Model *model, FIXED::Vector12 pos, FIXED::Vector12 rot) {}

}
