#include "renderer.hpp"

namespace ENGINE {

COMMON::ServiceLocator<Renderer> rendererInstance;

Renderer &Renderer::instance() {
    static Renderer *instance;
    #ifdef PLATFORM_PSX
    instance = new PSX::PSXRenderer();
    #endif
    return *instance;
}

}