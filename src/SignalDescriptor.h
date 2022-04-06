#pragma once
#include "toastbox/FileDescriptor.h"
#include <sys/event.h>

// SignalDescriptor: a FileDescriptor that can act as a signal between threads,
// and can participate in select() multiplexing
class SignalDescriptor : public Toastbox::FileDescriptor {
public:
    SignalDescriptor() {
#if __APPLE__
        int ir = kqueue();
//        printf("SignalDescriptor(%d)\n", (int)ir);
#elif __linux__
        int ir = eventfd(0, EFD_CLOEXEC);
#endif
        if (ir < 0) throw std::system_error(errno, std::generic_category());
        Toastbox::FileDescriptor fd(ir);
        swap(fd);
    }
    
//    ~SignalDescriptor() {
//        printf("~SignalDescriptor(%d)\n", (int)*this);
//    }
    
    void signal() {
        // Set our flag, which also ensures memory consistency (instead of relying on the fd signal).
        // Ie, when get() receives fd signal, it can safely assume that either val or err is set.
        _signaled.store(true);
        
#if __APPLE__
        struct kevent ev;
        EV_SET(&ev, 1, EVFILT_USER, EV_ADD, NOTE_TRIGGER, 0, NULL);
        int ir = 0;
        do ir = kevent(*this, &ev, 1, nullptr, 0, nullptr);
        while (ir<0 && errno==EINTR);
        assert(ir == 0);
#elif __linux__
        const uint64_t x = 1;
        ssize_t ir = 0;
        do ir = write(*this, &x, sizeof(x));
        while (ir<0 && errno==EINTR);
        assert(ir == sizeof(x));
#endif
    }
    
    bool signaled() const {
        return _signaled;
    }
    
    void wait() {
#if __APPLE__
        struct kevent ev;
        int ir = 0;
        do ir = kevent(*this, nullptr, 0, &ev, 1, nullptr);
        while (ir<0 && errno==EINTR);
        assert(ir == 1);
#elif __linux__
        uint64_t x = 0;
        ssize_t ir = 0;
        do ir = read(*this, &x, sizeof(x));
        while (ir<0 && errno==EINTR);
        assert(ir == sizeof(x));
#endif
    }

private:
    std::atomic<bool> _signaled = false;
};
