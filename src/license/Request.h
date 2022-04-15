#pragma once
#include "License.h"
#include "machine/Machine.h"

namespace License {

// Request: a license request-request, ie an HTTP request that's a request for a license
struct Request {
    // Required
    Machine::MachineId machineId;
    // Only used for registration (not trials)
    Email email;
    LicenseCode licenseCode;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Request, email, licenseCode, machineId);

// RequestResponse: the server's response to `Request` (above)
struct RequestResponse {
    std::string error;
    SealedLicense license;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestResponse, error, license);

} // namespace License
