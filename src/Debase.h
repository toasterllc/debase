#pragma once
#include <stdint.h>
#include "Version.h"

#define _DebaseVersion                      3
#define _DebaseFilename                     "debase"
#define _DebaseProductId                    "llc.toaster.debase"
#define _ToasterURL                         "https://toaster.llc"

inline const Version DebaseVersion          = _DebaseVersion;
inline const char* DebaseFilename           = _DebaseFilename;
inline const char* DebaseProductId          = _DebaseProductId;
