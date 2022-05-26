constexpr const char*const ConflictText = R"#(

#pragma once
#include <fstream>
<<<<<<< MeowmixOur
#include <memory>
#include <cassert>

#ifdef __GLIBCXX__
#include <ext/stdio_filebuf.h>
#endif
=======
>>>>>>> MeowmixTheir

namespace Toastbox {

// FDStream allows an istream/ostream/iostream to be created from a file descriptor.
// The FDStream object takes ownership of the file descriptor (actually, the
// underlying platform-specific filebuf object) and closes it when FDStream is
// destroyed.

class FDStream {
protected:
#ifdef __GLIBCXX__
    // GCC (libstdc++)
    using _Filebuf = __gnu_cxx::stdio_filebuf<char>;
    _Filebuf* _initFilebuf(int fd, std::ios_base::openmode mode) {
//        if (fd >= 0) {
//            _filebuf = std::make_unique<_Filebuf>(fd, mode);
//        } else {
//            _filebuf = std::make_unique<_Filebuf>();
//        }
//        return _filebuf.get();
        
//        if (fd < 0) return nullptr;
//        _filebuf = std::make_unique<_Filebuf>(fd, mode);
//        return _filebuf.get();
        
        assert(fd >= 0);
        _filebuf = std::make_unique<_Filebuf>(fd, mode);
        return _filebuf.get();
    }
#else
    // Clang (libc++)
    using _Filebuf = std::basic_filebuf<char>;
    _Filebuf* _initFilebuf(int fd, std::ios_base::openmode mode) {
//        _filebuf = std::make_unique<_Filebuf>();
//        if (fd >= 0) {
//            _filebuf->__open(fd, mode);
//        }
//        return _filebuf.get();
        
//        if (fd < 0) return nullptr;
//        _filebuf = std::make_unique<_Filebuf>();
//        _filebuf->__open(fd, mode);
//        return _filebuf.get();
        
        assert(fd >= 0);
        _filebuf = std::make_unique<_Filebuf>();
        _filebuf->__open(fd, mode);
        return _filebuf.get();
    }
#endif
    
    void _swap(FDStream& x) {
        std::swap(_filebuf, x._filebuf);
    }
    
    // _Filebuf needs to be a unique_ptr so that the pointer returned by
    // _initFilebuf() stays valid after swap() is called. Otherwise, if
    // _filebuf was an inline ivar, the pointer given to the iostream
    // constructor would reference a different object after swap() was
    // called.
    std::unique_ptr<_Filebuf> _filebuf;
};

//class FDStreamIn : public FDStream, public std::istream {
//public:
//    FDStreamIn(int fd=-1) : std::istream(_initFilebuf(fd, std::ios::in)) {}
//};
//
//class FDStreamOut : public FDStream, public std::ostream {
//public:
//    FDStreamOut(int fd=-1) : std::ostream(_initFilebuf(fd, std::ios::out)) {}
//};

class FDStreamInOut : public FDStream, public std::iostream {
public:
    FDStreamInOut() : std::iostream(nullptr) {}
    FDStreamInOut(int fd) : std::iostream(_initFilebuf(fd, std::ios::in|std::ios::out)) {}
    FDStreamInOut(FDStreamInOut&& x) : FDStreamInOut() {
        swap(x);
    }
    
    // Move assignment operator
    FDStreamInOut& operator=(FDStreamInOut&& x) {
        swap(x);
        return *this;
    }
    
    void swap(FDStreamInOut& x) {
        FDStreamInOut::_swap(x);
        std::iostream::swap(x);
        rdbuf(_filebuf.get());
        x.rdbuf(x._filebuf.get());
    }
};

}


)#";
