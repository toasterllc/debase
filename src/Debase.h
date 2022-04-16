#pragma once
#include <stdint.h>
#include "Version.h"

#if DebaseLicenseServer
#define inline // Go can't handle 'inline' below
#endif

#define _ToasterShortURL        "heytoaster.com"
#define _ToasterURL             "https://" _ToasterShortURL
#define _ToasterSupportEmail    "support@heytoaster.com"
#define _DebaseVersion          1

inline const char* ToasterDisplayURL    = _ToasterShortURL;
inline const char* ToasterSupportEmail  = _ToasterSupportEmail;

inline const Version DebaseVersion      = _DebaseVersion;
inline const char* DebaseProductId      = "com.heytoaster.debase";
inline const char* DebasePurchaseURL    = _ToasterURL "/debase";
inline const char* DebaseLicenseAPIURL  = "us-central1-capable-sled-346322.cloudfunctions.net/DebaseLicenseServer";

#if DebaseLicenseServer
inline const uint8_t DebaseKeyPrivate[] = {
    0xb3, 0xab, 0xbe, 0xc7, 0xfe, 0xe3, 0x1e, 0x1a,
    0xd4, 0xa4, 0x4e, 0xde, 0xfa, 0xf2, 0xc4, 0x4a,
    0xa5, 0x67, 0xdb, 0x03, 0x36, 0x32, 0x35, 0xa2,
    0xfa, 0xe4, 0x17, 0xb3, 0x15, 0x90, 0x66, 0x81,
};
#endif // DebaseLicenseServer

inline const uint8_t DebaseKeyPublic[] = {
    0x9e, 0xf7, 0x3a, 0x4c, 0xab, 0x98, 0x6e, 0x0b,
    0x98, 0xae, 0xa8, 0x64, 0xb9, 0x4c, 0x18, 0xca,
    0x6e, 0x7b, 0xa1, 0x04, 0x44, 0x15, 0x55, 0x3f,
    0x1b, 0x79, 0x34, 0xba, 0x6c, 0x84, 0xec, 0xff,
};
