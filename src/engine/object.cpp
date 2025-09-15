#include "object.hpp"
#include "../app.hpp"
#include "physics.hpp"

namespace ENGINE {

void Object3D::init(const char *modelpath) {
//	file = g_app.fileprovider.openFile(modelpath);
   // model = GFX::loadModel(file->fdata); 
    pos = rot = vel = accel = {0, 0, 0};
}

void Object3D::update(void) {
  // PHYSICS::actGravity3D(this);
}

void Object3D::render(void) {
//    g_app.renderer.drawModel(model, pos, rot);
}

void Object3D::free(void) {
  //  file->close();
//    freeModel(model);
}


}