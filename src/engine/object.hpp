#pragma once

#include "../gpu.hpp"

namespace ENGINE {

class Object3D {
public:
    void init(const char *modelpath);

    void update(void);
    void render(void);
        //Object3D todo deconstructor
private:
    const GFX::Model *model;
    GTEVector32 pos, rot;
};

//todo
class Object2D {
public:

private:
//    GFX::TextureInfo tex;
};

}