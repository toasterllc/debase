## Build for macOS

### Build libgit2

    cd lib/libgit2
    mkdir build-macos && cd build-macos
    cmake -DBUILD_SHARED_LIBS=OFF -DUSE_HTTPS=OFF ..
    make clean ; make git2 -j8

### Build ncurses

    cd lib/ncurses
    ./configure --prefix=/usr --enable-widec
    make -j8

### Build debase

- Open debase.xcodeproj
- Build



## Build for Linux

### Build libgit2



### Build ncurses



### Build debase

    cd debase
    make
