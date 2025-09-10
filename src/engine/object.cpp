#include "object.hpp"
#include "../app.hpp"

extern const uint8_t g_mymodel[];

namespace ENGINE {

void Object3D::init(const char *modelpath) {
    //todo read from cd
    model = GFX::loadModel(g_mymodel); 
    pos = {0, 0, 0};
    rot = {0, 0, 0};
}

void Object3D::setPos(const GTEVector32& p) {
    pos = p;
}

void Object3D::setRot(const GTEVector32& r) {
    rot = r;
}

void Object3D::update(void) {
    //todo physics
}

void Object3D::render(void) {
    g_app.renderer.drawModel(model, pos, rot);
}


}