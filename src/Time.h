#pragma once
#include <chrono>

namespace Time {

inline std::string RelativeTimeDisplayString(uint64_t past) {
    using namespace std;
    using seconds   = chrono::duration<int64_t, std::ratio<1>>;
    using minutes   = chrono::duration<int64_t, std::ratio<60>>;
    using hours     = chrono::duration<int64_t, std::ratio<3600>>;
    using days      = chrono::duration<int64_t, std::ratio<86400>>;
    using weeks     = chrono::duration<int64_t, std::ratio<604800>>;
    using months    = chrono::duration<int64_t, std::ratio<2629746>>;
    using years     = chrono::duration<int64_t, std::ratio<31556952>>;
    
    uint64_t current = time(nullptr);
    if (past > current) return "future";
    seconds delta(current-past);
    
    // Years
    int64_t d = chrono::duration_cast<years>(delta).count();
    if (d >= 1) return std::to_string(d)+"y ago";
    // Months
    d = chrono::duration_cast<months>(delta).count();
    if (d >= 1) return std::to_string(d)+"m ago";
    // Weeks
    d = chrono::duration_cast<weeks>(delta).count();
    if (d >= 1) return std::to_string(d)+"w ago";
    // Days
    d = chrono::duration_cast<days>(delta).count();
    if (d >= 1) return std::to_string(d)+"d ago";
    // Hours
    d = chrono::duration_cast<hours>(delta).count();
    if (d >= 1) return std::to_string(d)+"h ago";
    // Minutes
    d = chrono::duration_cast<minutes>(delta).count();
    if (d >= 1) return std::to_string(d)+"m ago";
    // Seconds
    d = chrono::duration_cast<seconds>(delta).count();
    if (d >= 10) return std::to_string(d)+"s ago";
    return "now";
}

} // namespace RelativeTime
