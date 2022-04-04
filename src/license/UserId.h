#pragma once
#include <sstream>
#include "Debase.h"
#include "SHA512Half.h"

namespace License {

using UserId = std::string;

inline std::string UserIdForEmail(std::string_view email) {
    std::stringstream s;
    s << DebaseProductId << ":" << email;
    return SHA512Half::StringForHash(SHA512Half::Calc(s.str()));
    
}

} // namespace License
