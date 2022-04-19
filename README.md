## Build for macOS

### Build libgit2

    cd debase/lib/libgit2
    mkdir build-mac && cd build-mac
    cmake -DBUILD_SHARED_LIBS=OFF -DUSE_HTTPS=OFF ..
    make clean ; make -j8 git2

### Build ncurses

    cd debase/lib/ncurses
    ./configure --prefix=/usr --enable-widec
    make -j8

### Build libcurl

#### Release build

    cd debase/lib/libcurl
    ./configure --prefix=$PWD/build --disable-shared --with-secure-transport --disable-debug --disable-curldebug --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --without-brotli --without-zstd --without-libpsl --without-libgsasl --without-librtmp --without-winidn
    make -j8 install-strip

#### Debug build

    cd debase/lib/libcurl
    ./configure --prefix=$PWD/build --disable-shared --with-secure-transport --enable-debug --disable-optimize --enable-curldebug --disable-symbol-hiding --disable-ldap
    make install -j8

### Build debase

- Open debase.xcodeproj
- Build



## Build for Linux

### Build libgit2

    cd debase/lib/libgit2
    mkdir build-linux && cd build-linux
    cmake -DBUILD_SHARED_LIBS=OFF -DUSE_HTTPS=OFF ..
    make clean ; make -j8 git2

### Build ncurses

    cd debase/lib/ncurses
    ./configure --prefix=/usr --enable-widec
    make -j8

### Build debase

    cd debase
    make
