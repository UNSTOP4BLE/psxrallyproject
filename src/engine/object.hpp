#pragma once

#include "../gpu.hpp"
#include "fs/file.hpp"

namespace ENGINE {

class Object3D {
public:
    void init(const char *modelpath);

    void update(void);
    void render(void);
    
    void free(void);
    FIXED::Vector12 pos, rot, vel, accel;
private:
    GFX::Model *model;
    ENGINE::FS::File* file;
};

//todo
class Object2D {
public:

private:
//    GFX::TextureInfo tex;
};

}