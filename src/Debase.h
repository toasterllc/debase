#pragma once
#include <stdint.h>
#include "Version.h"

#define _DebaseVersion                      2
#define _DebaseFilename                     "debase"
#define _DebaseProductId                    "llc.toaster.debase"
#define _ToasterURL                         "https://toaster.llc"

inline const Version DebaseVersion          = _DebaseVersion;
inline const char* DebaseFilename           = _DebaseFilename;
inline const char* DebaseProductId          = _DebaseProductId;
inline const char* DebaseDownloadURL        = _ToasterURL "/#debase-download";
inline const char* DebaseCurrentVersionURL  = _ToasterURL "/debase/version";
