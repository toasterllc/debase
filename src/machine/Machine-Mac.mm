#import "Machine.h"
#import <Foundation/Foundation.h>
#import <IOKit/IOKitLib.h>
#import <sys/utsname.h>
#import "lib/toastbox/SendRight.h"

namespace Machine {

std::string _SerialGet() {
    using namespace Toastbox;
    @autoreleasepool {
        SendRight platformExpert(SendRight::NoRetain, IOServiceGetMatchingService(kIOMasterPortDefault, IOServiceMatching("IOPlatformExpertDevice")));
        if (!platformExpert.valid()) throw std::runtime_error("IOServiceGetMatchingService failed");
        
        NSString* serial = CFBridgingRelease(IORegistryEntryCreateCFProperty(platformExpert, CFSTR(kIOPlatformSerialNumberKey), nullptr, 0));
        if (!serial || ![serial length]) throw std::runtime_error("IORegistryEntryCreateCFProperty failed");
        
        return [serial UTF8String];
    }
}

MachineId MachineIdCalc(std::string_view domain) noexcept {
    try {
        const std::string str = std::string(domain) + ":" + _SerialGet();
        SHA512Half::Hash hash = SHA512Half::Calc(str);
        return SHA512Half::StringForHash(hash);
    } catch (...) {}
    
    return MachineIdBasicCalc(domain);
}

MachineInfo MachineInfoCalc() noexcept {
    struct utsname un;
    int ir = uname(&un);
    const char* unameStr = (!ir ? un.version : nullptr);
    const char* osVersionStr = [[[NSProcessInfo processInfo] operatingSystemVersionString] UTF8String];
    return std::string("Mac OS: ") + (osVersionStr ? osVersionStr : "<unknown>") + "; uname: " + (unameStr ? unameStr : "<unknown>");
}

} // namespace Machine
