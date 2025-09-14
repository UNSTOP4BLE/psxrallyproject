#pragma once

#include "../fixed.hpp"
#include "object.hpp"

namespace ENGINE::PHYSICS {
constexpr FIXED::Fixed12 G = FIXED::makeFixed<12>(98, 10);
void actGravity3D(Object3D *obj);

}