#pragma once
#include <list>
#include "Window.h"

namespace UI {

class CursorState {
public:
    CursorState(bool visible, Point pos={}) : _id(_IdCurrent) {
        _IdCurrent++;
        
        _CursorState state = {
            .id = _id,
            .visible = visible,
            .position = pos,
        };
        
        _States.push_back(state);
        _Update();
    }
    
    
//    static CursorState Set(bool visible, Point pos={}) {
//        _Id id = _IdCurrent;
//        _IdCurrent++;
//        
//        _CursorState state = {
//            .id = id,
//            .visible = visible,
//            .position = pos,
//        };
//        
//        _States.push_back(state);
//        _Update();
//        return CursorState(id);
//    }
    
//    CursorState(bool visible, const Point& pos) :
//    _visible(visible), _position(pos) {}
//    
//    CursorState(bool visible) {
//        set(visible);
//    }
    
    CursorState() {}
    CursorState(const CursorState& x) = delete;
    CursorState(CursorState&& x) {
        _swap(x);
    }
    
    ~CursorState() {
        if (_id) {
            restore();
        }
    }
    
    // Move assignment operator
    CursorState& operator=(CursorState&& x) {
        _swap(x);
        return *this;
    }
    
    operator bool() { return _id; }
    
    void restore() {
        assert(_id);
        _Remove(_id);
        _id = 0;
    }
    
//    CursorState(bool visible, const Point& pos) :
//    _visible(visible), _position(pos) {}
//    
//    CursorState(bool visible) {
//        set(visible);
//    }
//    
//    CursorState(const CursorState& x) = delete;
//    CursorState(CursorState&& x) = delete;
//    
//    ~CursorState() {
//        if (_prev) {
//            restore();
//        }
//    }
//    
//    void set(bool visible) {
//        assert(!_prev);
//        _prev = curs_set(visible ? 1 : 0);
//    }
//    
//    void restore() {
//        assert(_prev);
//        curs_set(*_prev);
//        _prev = std::nullopt;
//    }
//    
//    operator bool() { return _prev.has_value(); }
//    
//private:
//    bool _visible = false;
//    Point _position;
    
private:
    using _Id = uint64_t;
    
    struct _CursorState {
        _Id id = 0;
        bool visible = false;
        Point position;
    };
    
    static inline _Id _IdCurrent = 1;
    static inline std::list<_CursorState> _States;
    
    static void _Update() {
        if (_States.empty()) return;
        const _CursorState& state = _States.back();
        ::curs_set(state.visible);
        ::move(state.position.y, state.position.x);
    }
    
    static void _Remove(_Id id) {
        for (auto it=_States.rbegin(); it!=_States.rend(); it++) {
            if ((*it).id == id) {
                _States.erase(std::next(it).base());
                _Update();
                return;
            }
        }
        // Programmer error if we try to pop an element that doesn't exist
        abort();
    }
    
    CursorState(_Id id) : _id(id) {}
    
    void _swap(CursorState& x) {
        std::swap(_id, x._id);
    }
    
    _Id _id = 0;
};

} // namespace UI
