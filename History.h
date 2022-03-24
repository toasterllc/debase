#pragma once
#include <deque>

template <typename T>
class T_History {
public:
    const T& get() const {
        return _current;
    }
    
    void set(const T& x) {
        _current = x;
    }
    
    void clear() {
        _prev.clear();
        _next.clear();
    }
    
    void push(const T& x) {
        _next = {};
        _prev.push_front(_current);
        _current = x;
    }
    
    void prev() {
        assert(!begin());
        _next.push_front(_current);
        _current = _prev.front();
        _prev.pop_front();
    }
    
    void next() {
        assert(!end());
        _prev.push_front(_current);
        _current = _next.front();
        _next.pop_front();
    }
    
    bool begin() const { return _prev.empty(); }
    bool end() const { return _next.empty(); }
    
//private:
    T _current = {};
    std::deque<T> _prev;
    std::deque<T> _next;
};
