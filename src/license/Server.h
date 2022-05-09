#pragma once
#include "License.h"
#include "machine/Machine.h"
#include "lib/nlohmann/json.h"

namespace License::Server {

struct CommandLicenseLookup {
    struct _Payload {
        Email email;
        LicenseCode licenseCode;
        Machine::MachineId machineId;
        Machine::MachineInfo machineInfo;
    };
    
    std::string type = "LookupLicense";
    _Payload payload;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::_Payload, email, licenseCode, machineId, machineInfo);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);

struct CommandTrialLookup {
    struct _Payload {
        Machine::MachineId machineId;
        Machine::MachineInfo machineInfo;
    };
    
    std::string type = "LookupTrial";
    _Payload payload;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::_Payload, machineId, machineInfo);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);

struct ReplyLicenseLookup {
    struct _Payload {
        SealedLicense license;
    };
    
    std::string error;
    _Payload payload;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ReplyLicenseLookup::_Payload, license);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ReplyLicenseLookup, error, payload);

} // namespace License::Server















//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct CommandLicenseLookup {
//    struct _Payload {
//        Email email;
//        LicenseCode licenseCode;
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(_Payload, email, licenseCode, machineId, machineInfo);
//    };
//    
//    std::string type = "LookupLicense";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::payload, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    struct _Payload {
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(_Payload, machineId, machineInfo);
//    };
//    
//    std::string type = "LookupTrial";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::payload, machineId, machineInfo);
//
//struct ReplyLicenseLookup {
//    std::string error;
//    SealedLicense license;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ReplyLicenseLookup, error, license);
//
//} // namespace License::Server










//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct CommandLicenseLookup {
//    std::string type = "LookupLicense";
//    struct {
//        Email email;
//        LicenseCode licenseCode;
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(decltype(this), email, licenseCode, machineId, machineInfo);
//    } payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::payload, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    std::string type = "LookupTrial";
//    struct {
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(decltype(this), machineId, machineInfo);
//    } payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::payload, machineId, machineInfo);
//
//} // namespace License::Server


















//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct CommandLicenseLookup {
//    struct _Payload {
//        Email email;
//        LicenseCode licenseCode;
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(_Payload, email, licenseCode, machineId, machineInfo);
//    };
//    
//    std::string type = "LookupLicense";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::payload, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    struct _Payload {
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//        
//        NLOHMANN_DEFINE_TYPE_INTRUSIVE(_Payload, machineId, machineInfo);
//    };
//    
//    std::string type = "LookupTrial";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::payload, machineId, machineInfo);
//
//} // namespace License::Server







//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct CommandLicenseLookup {
//    struct _Payload {
//        Email email;
//        LicenseCode licenseCode;
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//    };
//    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(_Payload, email, licenseCode, machineId, machineInfo);
//    
//    std::string type = "LookupLicense";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::payload, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    struct _Payload {
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//    };
//    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(_Payload, machineId, machineInfo);
//    
//    std::string type = "LookupTrial";
//    _Payload payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::payload, machineId, machineInfo);
//
//} // namespace License::Server







//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct Command {
//    std::string type;
//    nlohmann::json payload;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Command, type, payload);
//
//struct CommandLicenseLookup {
//    Email email;
//    LicenseCode licenseCode;
//    Machine::MachineId machineId;
//    Machine::MachineInfo machineInfo;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    Machine::MachineId machineId;
//    Machine::MachineInfo machineInfo;
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, machineId, machineInfo);
//
//} // namespace License::Server








//#pragma once
//#include "License.h"
//#include "machine/Machine.h"
//#include "lib/nlohmann/json.h"
//
//namespace License::Server {
//
//struct CommandLicenseLookup {
//    std::string type;
//    struct {
//        Email email;
//        LicenseCode licenseCode;
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//    } payload;
//    
//    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(decltype(payload), email, licenseCode, machineId, machineInfo);
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandLicenseLookup::payload, email, licenseCode, machineId, machineInfo);
//
//struct CommandTrialLookup {
//    std::string type;
//    struct {
//        Machine::MachineId machineId;
//        Machine::MachineInfo machineInfo;
//    } payload;
//    
//    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(decltype(payload), machineId, machineInfo);
//};
//
//NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup, type, payload);
////NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CommandTrialLookup::payload, machineId, machineInfo);
//
//} // namespace License::Server
