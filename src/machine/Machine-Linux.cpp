#include <sstream>
#include <sys/utsname.h>
#include "Machine.h"

namespace Machine {

MachineId MachineIdCalc(std::string_view domain) noexcept {
    return MachineIdBasicCalc(domain);
}

MachineInfo MachineInfoCalc() noexcept {
    std::stringstream stream;
    struct utsname un;
    int ir = uname(&un);
    if (ir) return "";
    stream <<        un.sysname;
    stream << " " << un.release;
    stream << " " << un.version;
    stream << " " << un.machine;
    return stream.str();
}

} // namespace Machine
