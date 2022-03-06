#pragma once

template <typename T, auto& T_Deleter>
struct RefCounted : public std::shared_ptr<T> {
    RefCounted() : std::shared_ptr<T>() {}
    RefCounted(const T& a) : std::shared_ptr<T>(new T(a), _Deleter) {}
private:
    static void _Deleter(T* t) { T_Deleter(*t); }
};
