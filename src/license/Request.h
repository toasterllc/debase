#pragma once
#include "UserId.h"
#include "MachineId.h"
#include "LicenseCode.h"
#include "License.h"

namespace License {

// Request: a license request-request, ie an HTTP request that's a request for a license
struct Request {
    // Required
    MachineId machineId;
    // Only used for registration (not trials)
    UserId userId;
    LicenseCode licenseCode;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Request, userId, licenseCode, machineId);

// RequestResponse: the server's response to `Request` (above)
struct RequestResponse {
    std::string error;
    SealedLicense license;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestResponse, error, license);

} // namespace License
