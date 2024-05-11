# debase

![debase screen recording](./readme-copy.gif)

`debase` makes editing git history easier by allowing you to move / copy / edit / delete commits with a mouse.

Full details: [toaster.llc/debase](https://toaster.llc/debase).

## Download

Compiled binaries are available at [toaster.llc/debase](https://toaster.llc/debase).

## Build for macOS

#### Build
    # x86_64 + arm64
    make -j8
    
    # x86_64 only
    make -j8 ARCHS=x86_64

#### Notarize Binary

    make notarize

#### Check status of notarization process

    make notarize-status

#### Test local binary notarization

    make notarize-test

## Build for Linux

    make -j8

