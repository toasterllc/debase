#pragma once
#include <memory>

template <typename T, auto& T_Deleter>
struct RefCounted : std::shared_ptr<T> {
    using std::shared_ptr<T>::shared_ptr;
    
    // Pointer types
    template<
    typename _T = T,
    typename std::enable_if_t<std::is_pointer_v<_T>, int> = 0
    >
    RefCounted(const T& t) : std::shared_ptr<T>((t ? new T(t) : nullptr), _Deleter) {}
    
    // Non-pointer types
    template<
    typename _T = T,
    typename std::enable_if_t<!std::is_pointer_v<_T>, int> = 0
    >
    RefCounted(const T& t) : std::shared_ptr<T>(new T(t), _Deleter) {}

private:
    static void _Deleter(T* t) { if (t) T_Deleter(*t); }
};
