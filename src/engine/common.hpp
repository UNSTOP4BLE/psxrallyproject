#pragma once 

#include <stdint.h>

#include "templates.hpp"

namespace ENGINE::COMMON {

    constexpr int SCREEN_WIDTH  = 320;
    constexpr int SCREEN_HEIGHT = 240;

    //only useful on ps1
    constexpr int DMA_MAX_CHUNK_SIZE  = 16;
    constexpr int CHAIN_BUFFER_SIZE   = 4104;
    constexpr int ORDERING_TABLE_SIZE = 1024;

    //common definitions
    using RECT32 = ENGINE::TEMPLATES::RECT<int32_t>;
    using XY32   = ENGINE::TEMPLATES::XY<int32_t>;

    class Scene {
    public:
        inline Scene(void) {}
        virtual void update(void) {}
        virtual void draw(void) {}
        virtual ~Scene(void) {}
    };

    void hexDump(const void* data, uint32_t size);
} //namespace ENGINE::COMMON
