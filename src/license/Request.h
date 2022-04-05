#pragma once
#include "UserId.h"
#include "MachineId.h"
#include "RegisterCode.h"
#include "License.h"

namespace License {

// Request: a license request-request, ie an HTTP request that's a request for a license
struct Request {
    UserId userId;              // Populated when registered (ie !trial)
    RegisterCode registerCode;  // Populated when registered (ie !trial)
    MachineId machineId;        // Always populated
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Request, userId, registerCode, machineId);

// RequestResponse: the server's response to `Request` (above)
struct RequestResponse {
    std::string error;
    SealedLicense license;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(RequestResponse, error, license);

} // namespace License
