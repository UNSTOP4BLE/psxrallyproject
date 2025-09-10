#include "object.hpp"
#include "../app.hpp"

namespace ENGINE {

void Object3D::init(const char *modelpath) {
	file = g_app.fileprovider.openFile(modelpath);
    model = GFX::loadModel(file->fdata); 
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

void Object3D::free(void) {
    file->close();
    freeModel(model);
}


}