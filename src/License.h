#pragma once
#include "lib/nlohmann/json.h"

extern "C" {
#include "lib/c25519/src/edsign.h"
}

namespace License {

#if DebaseServer
static constexpr uint8_t KeySecret[] = {
    0xb3, 0xab, 0xbe, 0xc7, 0xfe, 0xe3, 0x1e, 0x1a,
    0xd4, 0xa4, 0x4e, 0xde, 0xfa, 0xf2, 0xc4, 0x4a,
    0xa5, 0x67, 0xdb, 0x03, 0x36, 0x32, 0x35, 0xa2,
    0xfa, 0xe4, 0x17, 0xb3, 0x15, 0x90, 0x66, 0x81
};
static_assert(sizeof(KeySecret) == EDSIGN_SECRET_KEY_SIZE);
#endif // DebaseServer

static constexpr uint8_t KeyPublic[] = {
    0x9e, 0xf7, 0x3a, 0x4c, 0xab, 0x98, 0x6e, 0x0b,
    0x98, 0xae, 0xa8, 0x64, 0xb9, 0x4c, 0x18, 0xca,
    0x6e, 0x7b, 0xa1, 0x04, 0x44, 0x15, 0x55, 0x3f,
    0x1b, 0x79, 0x34, 0xba, 0x6c, 0x84, 0xec, 0xff
};
static_assert(sizeof(KeyPublic) == EDSIGN_PUBLIC_KEY_SIZE);

using MachineId = std::string;

struct License {
    std::string payload;
    std::string signature;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(License, payload, signature);

struct LicensePayload {
    uint32_t version = 0;
    MachineId machineId;
    uint64_t expiration = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LicensePayload, version, machineId, expiration);

struct Context {
    uint32_t version = 0;
    MachineId machineId;
    uint64_t time = 0;
};

enum class ValidateResult {
    Invalid,
    InvalidSignatureSize,
    InvalidSignature,
    InvalidPayload,
    InvalidVersion,
    InvalidMachineId,
    Expired,
    Valid,
};

// Validate: determines whether license is valid for a given context
inline ValidateResult Validate(const License& license, const Context& ctx) {
    // Validate that the signature is the correct size
    if (license.signature.size() != EDSIGN_SIGNATURE_SIZE) return ValidateResult::InvalidSignatureSize;
    
    // Validate that the license signature matches the license payload
    // This verifies that the license came from our trusted server
    bool ok = edsign_verify((const uint8_t*)license.signature.c_str(), KeyPublic,
        (const uint8_t*)license.payload.c_str(), license.payload.size());
    if (!ok) return ValidateResult::InvalidSignature;
    
    // Decode the payload
    LicensePayload payload;
    try {
        nlohmann::json j = license.payload;
        j.get_to(payload);
    
    } catch (...) {
        // Invalid data
        return ValidateResult::InvalidPayload;
    }
    
    // Check if the licensed version is lower than the current version
    if (payload.version < ctx.version) return ValidateResult::InvalidVersion;
    // Check that the licensed machine id is the current machine id
    if (payload.machineId != ctx.machineId) return ValidateResult::InvalidMachineId;
    // Check if the license has expired
    if (payload.expiration && payload.expiration<ctx.time) return ValidateResult::Expired;
    
    return ValidateResult::Valid;
}

// - when a trial license expires, just delete the license from the state file.
// - don't check with the server every launch.
// - also, the server should respond with the same trial license until it's expired, and then the server
//   should just return an error, rather than return the expired license.
// - implement a 'grace' period in the debase binary so that it only ends the trial X minutes after
//   the expiration time. this is so that once debase notifies the user that the trial has ended,
//   the server is guaranteed (within reason) to no longer issue the expired license (to prevent
//   machine time rollback attacks). 
//
//
// - with the above strategy:
//
//   - we only do network IO when initiating the trial, not every launch
//   
//   - we have reasonable security against rolling back the machine's time -- that'll only work if you
//     save the license before it expires _and_ you roll back the machine's time.

} // namespace License
