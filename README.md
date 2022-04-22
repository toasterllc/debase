## Build for macOS

### Build libgit2

    cd lib/libgit2
    mkdir build-mac && cd build-mac
    cmake -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=10.15 -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF -DUSE_HTTPS=OFF ..
    make clean ; make -j8 git2

### Build ncurses

    cd lib/ncurses
    ./configure CFLAGS='-mmacosx-version-min=10.15 -arch arm64 -arch x86_64' --prefix=/usr --enable-widec
    make -j8

### Build libcurl

    cd lib/libcurl
    ./configure CFLAGS='-mmacosx-version-min=10.15 -arch arm64 -arch x86_64' --prefix=$PWD/build --with-secure-transport --disable-shared --disable-debug --disable-curldebug --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --without-brotli --without-zstd --without-libpsl --without-libgsasl --without-librtmp --without-winidn
    make -j8 install-strip

### Build debase

- Open debase.xcodeproj
- Build



## Build for Linux

### Build libgit2

    cd lib/libgit2
    mkdir build-linux && cd build-linux
    cmake -DBUILD_SHARED_LIBS=OFF -DUSE_HTTPS=OFF ..
    make clean ; make -j8 git2

### Build ncurses

    cd lib/ncurses
    ./configure --prefix=/usr --enable-widec
    make -j8

### Build libcurl

    cd lib/libcurl
    ./configure --prefix=$PWD/build --with-openssl --disable-shared --disable-debug --disable-curldebug --disable-ftp --disable-file --disable-ldap --disable-ldaps --disable-rtsp --disable-proxy --disable-dict --disable-telnet --disable-tftp --disable-pop3 --disable-imap --disable-smb --disable-smtp --disable-gopher --disable-mqtt --disable-manual --without-brotli --without-zstd --without-libpsl --without-libgsasl --without-librtmp --without-winidn
    make -j8 install-strip

### Build debase

    cd debase
    make -j8
