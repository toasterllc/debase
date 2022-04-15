#include "Machine.h"

namespace Machine {

MachineId MachineIdCalc(std::string_view domain) noexcept {
    return MachineIdBasicCalc(domain);
}

} // namespace Machine
