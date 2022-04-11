#pragma once
#include <sstream>
#include "SHA512Half.h"

namespace License {

using UserId = std::string;

inline UserId UserIdForEmail(std::string_view domain, std::string_view email) {
    std::stringstream s;
    s << domain << ":" << email;
    return SHA512Half::StringForHash(SHA512Half::Calc(s.str()));
}

} // namespace License
