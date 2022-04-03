#pragma once
#include "UI.h"
//#include <os/log.h>

namespace UI {

class CursorState {
public:
    static void Draw() {
        if (_States.empty()) return;
        const _CursorState& state = _States.back();
//        os_log(OS_LOG_DEFAULT, "Cursor = {%d %d}", state.position.x, state.position.y);
        ::move(state.position.y, state.position.x);
//        ::refresh();
        ::curs_set(state.visible);
    }
    
    CursorState(bool visible, Point pos={}) : _id(_IdCurrent) {
        _IdCurrent++;
        
        _CursorState state = {
            .id = _id,
            .visible = visible,
            .position = pos,
        };
        
        _States.push_back(state);
    }
    
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
    CursorState& operator =(CursorState&& x) {
        _swap(x);
        return *this;
    }
    
    operator bool() { return _id; }
    
    void restore() {
        assert(_id);
        _Remove(_id);
        _id = 0;
    }
    
private:
    using _Id = uint64_t;
    
    struct _CursorState {
        _Id id = 0;
        bool visible = false;
        Point position;
    };
    
    static inline _Id _IdCurrent = 1;
    static inline std::list<_CursorState> _States;
    
    static void _Remove(_Id id) {
        for (auto it=_States.begin(); it!=_States.end(); it++) {
            if ((*it).id == id) {
                _States.erase(it);
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
