#pragma once

#include <stdint.h>

namespace ENGINE::TEMPLATES {
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
}