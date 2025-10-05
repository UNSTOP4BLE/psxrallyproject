#pragma once 

#include <stdint.h>

namespace ENGINE::COMMON {

constexpr int SCREEN_WIDTH  = 320;
constexpr int SCREEN_HEIGHT = 240;

//only useful on ps1
constexpr int DMA_MAX_CHUNK_SIZE  = 16;
constexpr int CHAIN_BUFFER_SIZE   = 4104;
constexpr int ORDERING_TABLE_SIZE = 1024;

template<typename T>
struct [[gnu::packed]] RECT {
    T x, y, w, h;

    RECT() = default;
    RECT(T _x, T _y, T _w, T _h) : x(_x), y(_y), w(_w), h(_h) {}
};

template<typename T>
struct [[gnu::packed]] XY {
    T x, y;

    XY() = default;
    XY(T _x, T _y) : x(_x), y(_y) {}
};

template <typename T>
class UniquePtr {
public:
    explicit UniquePtr(T* ptr = 0) : ptr_(ptr) {}
    ~UniquePtr() { reset(); }

    UniquePtr(const UniquePtr&) = delete;
    UniquePtr& operator=(const UniquePtr&) = delete;

    UniquePtr(UniquePtr&& other) {
        ptr_ = other.ptr_;
        other.ptr_ = 0;
    }

    UniquePtr& operator=(UniquePtr&& other) {
        if (this != &other) {
            reset();
            ptr_ = other.ptr_;
            other.ptr_ = 0;
        }
        return *this;
    }

    T* get() const { return ptr_; }
    T& operator*() const { return *ptr_; }
    T* operator->() const { return ptr_; }
    operator bool() const { return ptr_ != 0; }

    void reset(T* new_ptr = 0) {
        if (ptr_) delete ptr_;
        ptr_ = new_ptr;
    }

    T* release() {
        T* tmp = ptr_;
        ptr_ = 0;
        return tmp;
    }

private:
    T* ptr_;
};

template <typename T>
class ServiceLocator {
public:
    T* get() { return service_; }

    void provide(T* service) {service_ = service;}

private:
    T* service_;
};

//common definitions
using RECT32 = RECT<int32_t>;
using XY32   = XY<int32_t>;

class Scene {
public:
    inline Scene(void) {}
    virtual void update(void) {}
    virtual void draw(void) {}
    virtual ~Scene(void) {}
};

} //namespace ENGINE::COMMON