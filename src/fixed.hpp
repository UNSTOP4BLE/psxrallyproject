#pragma once
#include <stdint.h>

namespace FIXED {

template<int SHIFT>
struct Num {
    int32_t value;
    static constexpr int ONE = 1 << SHIFT;

    constexpr Num() : value(0) {}
    constexpr Num(int i) : value(i << SHIFT) {}
    constexpr Num(int32_t raw, bool) : value(raw) {} // internal raw constructor

    static constexpr Num fromRaw(int32_t raw) { return Num(raw, true); }
    constexpr int toInt() const { return value >> SHIFT; }

    constexpr Num operator+(const Num& other) const { return fromRaw(value + other.value); }
    constexpr Num operator-(const Num& other) const { return fromRaw(value - other.value); }
    constexpr Num operator*(const Num& other) const {
        return fromRaw(static_cast<int32_t>((static_cast<int64_t>(value) * other.value) >> SHIFT));
    }
    constexpr Num operator/(const Num& other) const {
        return fromRaw(static_cast<int32_t>((static_cast<int64_t>(value) << SHIFT) / other.value));
    }
};

template<int SHIFT>
constexpr Num<SHIFT> makeFixed(int num, int den = 1) {
    return Num<SHIFT>::fromRaw((static_cast<int64_t>(num) << SHIFT) / den);
}

using Fixed16 = Num<16>;

} 