#pragma once

template <typename T>
struct Bitfield {
    using Bit = T;
    Bitfield() {}
    Bitfield(T val) : val(val) {}
    operator T&() { return val; };
    operator const T&() const { return val; };
    T val = 0;
};
