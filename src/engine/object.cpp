#include "object.hpp"

extern const uint8_t g_mymodel[];

namespace ENGINE {

void Object3D::init(const char *modelpath) {
    //todo read from cd
    GFX::loadModel(g_mymodel); 
}

void Object3D::update(void) {
    //todo physics
}

void Object3D::render(void) {
    //todo read from cd
    GFX::loadModel(g_mymodel); 
}


}