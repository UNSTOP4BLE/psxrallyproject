#pragma once
#include <stdint.h>

namespace FIXED {

template<int SHIFT>
struct Num {
    int32_t value;
    static constexpr int ONE = 1 << SHIFT;

    Num() : value(0) {}
    Num(int i) : value(i << SHIFT) {}

    static Num fromRaw(int32_t raw) { Num f; f.value = raw; return f; }
    int toInt() const { return value >> SHIFT; }

    Num operator+(const Num& other) const { return Num::fromRaw(value + other.value); }
    Num operator-(const Num& other) const { return Num::fromRaw(value - other.value); }
    Num operator*(const Num& other) const { 
        return Num::fromRaw(static_cast<int32_t>((static_cast<int64_t>(value) * other.value) >> SHIFT)); 
    }
    Num operator/(const Num& other) const { 
        return Num::fromRaw(static_cast<int32_t>((static_cast<int64_t>(value) << SHIFT) / other.value)); 
    }
};

} 
