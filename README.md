## macOS

### Build

#### x86_64 and arm64

    # x86_64 + arm64
    make -j8
    
    # x86_64 only
    make -j8 ARCHS=x86_64

### Notarize Binary

    make notarize

### Check status of notarization process

    make notarize-status

### Test local binary notarization

    make notarize-test

## Linux

### Build

    make -j8
