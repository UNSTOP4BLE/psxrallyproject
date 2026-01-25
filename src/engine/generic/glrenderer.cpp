#include "../renderer.hpp"
#include <glad/glad.h>
#include <SDL2/SDL.h>
#include <assert.h>

namespace ENGINE::GENERIC {

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

        auto err = gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress);
        assert(err);

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
