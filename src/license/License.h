#pragma once
#include "lib/nlohmann/json.h"
#include "lib/toastbox/IntForStr.h"
#include "UserId.h"
#include "MachineId.h"
#include "LicenseCode.h"

extern "C" {
#include "lib/c25519/src/edsign.h"
}

namespace License {

struct SealedLicense {
    std::string payload;
    std::string signature;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(SealedLicense, payload, signature);

struct License {
    UserId userId;              // Hashed from email
    LicenseCode licenseCode;    // Code supplied to user upon purchase
    MachineId machineId;        // Stable, unique identifier for the licensed machine
    uint32_t version = 0;       // Software version
    int64_t expiration = 0;     // Expiration (non-zero only for trial licenses)
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(License, userId, licenseCode, machineId, version, expiration);

struct Context {
    MachineId machineId;
    uint32_t version = 0;
    std::chrono::system_clock::time_point time;
};

enum class Status {
    Empty,
    InvalidSignatureSize,
    InvalidSignature,
    InvalidPayload,
    InvalidVersion,
    InvalidMachineId,
    Expired,
    Valid,
};

//struct Status : Bitfield<uint32_t> {
//    using Bitfield::Bitfield;
//    static constexpr Bit None                   = 0;
//    static constexpr Bit InvalidSignatureSize   = 1<<0;
//    static constexpr Bit InvalidSignature       = 1<<1;
//    static constexpr Bit InvalidPayload         = 1<<2;
//    static constexpr Bit InvalidVersion         = 1<<3;
//    static constexpr Bit InvalidMachineId       = 1<<4;
//    static constexpr Bit Expired                = 1<<5;
//    static constexpr Bit Valid                  = 1<<6; // Intentionally not 0 so that the Valid value isn't the most common value lying around RAM
//};

// Unseal: decodes a SealedLicense -> License, if the signature is valid
inline Status Unseal(const uint8_t* publicKey, const SealedLicense& sealed, License& license) {
    if (sealed.payload.empty()) return Status::Empty;
    // Validate that the signature is the correct size
    if (sealed.signature.size() != 2*EDSIGN_SIGNATURE_SIZE) return Status::InvalidSignatureSize;
    
    // Hex string -> bytes
    uint8_t sig[EDSIGN_SIGNATURE_SIZE];
    for (size_t i=0; i<EDSIGN_SIGNATURE_SIZE; i++) {
        const char s[3] = {sealed.signature[2*i],sealed.signature[(2*i)+1],0};
        sig[i] = Toastbox::IntForStr<uint8_t>(s, 16);
    }
    
    // Validate that the signature is valid for the payload
    // This verifies that the license came from our trusted server
    bool ok = edsign_verify(sig, publicKey,
        (const uint8_t*)sealed.payload.c_str(), sealed.payload.size());
    if (!ok) return Status::InvalidSignature;
    
    // Decode the payload
    try {
        nlohmann::json j = nlohmann::json::parse(sealed.payload);
        j.get_to(license);
    
    } catch (...) {
        // Invalid data
        return Status::InvalidPayload;
    }
    
    return Status::Valid;
}

// Validate: determines whether license is valid for a given `Context`
inline Status Validate(const Context& ctx, const License& license) {
    using namespace std::chrono;
    // Check if the licensed version is lower than the current version
    if (license.version < ctx.version) return Status::InvalidVersion;
    // Check that the licensed machine id is the current machine id
    if (license.machineId != ctx.machineId) return Status::InvalidMachineId;
    // Check if the license has expired
    if (license.expiration && system_clock::from_time_t(license.expiration)<ctx.time) return Status::Expired;
    return Status::Valid;
}

// - when a trial license expires, just delete the license from the state file.
// - don't check with the server every launch.
// - also, the server should respond with the same trial license until it's expired, and then the server
//   should just return an error, rather than return the expired license.
// - implement a 'grace' period in the debase binary so that it only ends the trial X minutes after
//   the expiration time. this is so that once debase notifies the user that the trial has ended,
//   the server is guaranteed (within reason) to no longer issue the expired license (to prevent
//   machine time-rollback attacks).
//
// - to help prevent time-rollback: determine the time using max(time(), modification/creation time of every file in current directory)
//
// - with the above strategy:
//
//   - we only do network IO when initiating the trial, not every launch
//   
//   - we have reasonable security against rolling back the machine's time -- that'll only work if you
//     save the license before it expires _and_ you roll back the machine's time.

// - have a troubleshooting mechanism to allow a license to be specified on the command line,
//   to support users who email that the trial isn't working, or requesting more time.
//   2 parts:
//     debase --machine    prints the machine id, which the user communicates to support@heytoaster.com
//     debase --license    sets the license, supplied by support@heytoaster.com

} // namespace License
