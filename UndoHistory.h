#pragma once
#include <deque>

template <typename T>
class T_UndoHistory {
public:
    const T& get() const {
        return _current;
    }
    
    void set(const T& x) {
        _current = x;
    }
    
    void clear() {
        _undo.clear();
        _redo.clear();
    }
    
    void push(const T& x) {
        _redo = {};
        _undo.push_front(x);
    }
    
    void undo() {
        assert(canUndo());
        _redo.push_front(_current);
        _current = _undo.front();
        _undo.pop_front();
    }
    
    void redo() {
        assert(canRedo());
        _undo.push_front(_current);
        _current = _redo.front();
        _redo.pop_front();
    }
    
    bool canUndo() const { return !_undo.empty(); }
    bool canRedo() const { return !_redo.empty(); }
    
//private:
    T _current = {};
    std::deque<T> _undo;
    std::deque<T> _redo;
};
