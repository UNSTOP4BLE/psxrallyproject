#pragma once 

#include "templates.hpp"

#include <stdint.h>

namespace ENGINE::COMMON {
    //common definitions
    using RECT32   = ENGINE::TEMPLATES::RECT<int32_t>;
    using XY32     = ENGINE::TEMPLATES::XY<int32_t>;
    using TRI32    = ENGINE::TEMPLATES::TRI<int32_t>;

    class Scene {
    public:
        inline Scene(void) {}
        virtual void update(void) {}
        virtual void draw(void) {}
        virtual ~Scene(void) {}
    };

    void hexDump(const void* data, uint32_t size);

    constexpr int min(int a, int b) {
        return (a < b) ? a : b;
    }

    constexpr int max(int a, int b) {
        return (a > b) ? a : b;
    }

    inline bool isBufferAligned(void *buf) {
        return !(uintptr_t(buf) % alignof(uint32_t));
    }
} //namespace ENGINE::COMMON
