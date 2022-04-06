#pragma once
#include <sys/socket.h>
#include <optional>
#include "SignalDescriptor.h"

template <typename Fn>
class Async {
public:
    using Val = typename std::invoke_result<Fn>::type;
    
    Async(Fn&& fn) {
        _state = std::make_shared<_State>();
        
        std::thread([=] {
            try {
                _state->val = fn();
            } catch (...) {
                _state->err = std::current_exception();
            }
            
            // Signal the fd
            _state->signal.signal();
        }).detach();
    }
    
    Val& get() {
        // Wait for the signal
        _state->signal.wait();
        // We must either have a value or an exception
        assert(_state->val || _state->err);
        if (_state->err) {
            std::rethrow_exception(_state->err);
        }
        return *_state->val;
    }
    
    const Toastbox::FileDescriptor& signal() {
        return _state->signal;
    }
    
    bool done() const {
        return _state->signal.signaled();
    }
    
private:
    struct _State {
        SignalDescriptor signal;
        std::exception_ptr err;
        std::optional<Val> val; // Optional so that a default constructor isn't required
    };
    using _StatePtr = std::shared_ptr<_State>;
    
    _StatePtr _state;
};
