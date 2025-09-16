#include "physics.hpp"

namespace ENGINE::PHYSICS {

void actGravity3D(Object3D *obj) {
    FIXED::Fixed12 DT = FIXED::makeFixed<12>(1, 60); // 1/60 FPS in Fixed12, just deltatime
    obj->accel.y += G;

    obj->vel.x += obj->accel.x * DT;
    obj->vel.y += obj->accel.y * DT;
    obj->vel.z += obj->accel.z * DT;

    obj->pos.x += obj->vel.x * DT;
    obj->pos.y += obj->vel.y * DT;
    obj->pos.z += obj->vel.z * DT;

    // Floor collision
    if (obj->pos.y.value > 64) {
        obj->pos.y -= obj->vel.y * DT;
        obj->vel.y = 0;
        obj->accel.y = 0;
    }
}


}