#pragma once

#include "../gpu.hpp"

namespace ENGINE {

class Object3D {
public:
    void init(const char *modelpath);

    void setPos(const GTEVector32& p);
    void setRot(const GTEVector32& r);
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