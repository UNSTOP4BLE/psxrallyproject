#pragma once

#include "../gpu.hpp"
#include "fs/file.hpp"

namespace ENGINE {

class Object3D {
public:
    void init(const char *modelpath);

    void setPos(const GTEVector32& p);
    void setRot(const GTEVector32& r);
    void update(void);
    void render(void);
    
    void free(void);
private:
    GFX::Model *model;
    ENGINE::FS::File* file;
    GTEVector32 pos, rot, vel, accel;
};

//todo
class Object2D {
public:

private:
//    GFX::TextureInfo tex;
};

}