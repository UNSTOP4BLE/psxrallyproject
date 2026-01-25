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

    //primary template (for single objects)
    template <typename T>
    class UniquePtr {
    public:
        explicit UniquePtr(T* ptr = nullptr) : ptr_(ptr) {}
        ~UniquePtr() { reset(); }

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this != &other) {
                reset();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        T* get() const { return ptr_; }
        T& operator*() const { return *ptr_; }
        T* operator->() const { return ptr_; }
        operator bool() const { return ptr_ != nullptr; }

        void reset(T* new_ptr = nullptr) {
            if (ptr_) delete ptr_;
            ptr_ = new_ptr;
        }

        T* release() {
            T* tmp = ptr_;
            ptr_ = nullptr;
            return tmp;
        }

    private:
        T* ptr_;
    };


    //partial specialization (for arrays)
    template <typename T>
    class UniquePtr<T[]> {
    public:
        explicit UniquePtr(T* ptr = nullptr) : ptr_(ptr) {}
        ~UniquePtr() { reset(); }

        UniquePtr(const UniquePtr&) = delete;
        UniquePtr& operator=(const UniquePtr&) = delete;

        UniquePtr(UniquePtr&& other) noexcept : ptr_(other.ptr_) {
            other.ptr_ = nullptr;
        }

        UniquePtr& operator=(UniquePtr&& other) noexcept {
            if (this != &other) {
                reset();
                ptr_ = other.ptr_;
                other.ptr_ = nullptr;
            }
            return *this;
        }

        T* get() const { return ptr_; }
        T& operator[](uint32_t index) const { return ptr_[index]; }
        operator bool() const { return ptr_ != nullptr; }

        void reset(T* new_ptr = nullptr) {
            if (ptr_) delete[] ptr_;
            ptr_ = new_ptr;
        }

        T* release() {
            T* tmp = ptr_;
            ptr_ = nullptr;
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