#include "renderer.hpp"

namespace ENGINE {

    TEMPLATES::ServiceLocator<Renderer> g_rendererInstance;

    Renderer &Renderer::instance() {
        static Renderer *instance;
        #ifdef PLATFORM_PSX
        instance = new PSX::PSXRenderer();
        #else
        instance = new GENERIC::GLRenderer();
        #endif
        return *instance;
    }

}