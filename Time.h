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
    
//    // Seconds
//    int64_t d = chrono::duration_cast<seconds>(delta).count();
//    if (d < 10) return "now";
//    if (d < 60) return std::to_string(delta.count())+"s";
//    // Minutes
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 60) return std::to_string(d)+"m";
//    // Hours
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 24) return std::to_string(d)+"h";
//    // Days
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 7) return std::to_string(d)+"d";
//    // Weeks
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 52) return std::to_string(d)+"w";
//    // Months
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 52) return std::to_string(d)+"w";
//    // Years
//    d = chrono::duration_cast<minutes>(delta).count();
//    if (d < 52) return std::to_string(d)+"w";
    
    
    
//    const uint64_t delta = current-past;
//    uint64_t d = current-past;
//    
//    // Seconds
//    if (d < 10) return "now";
//    if (d < 60) return std::to_string(d)+"s";
//    d /= 60;
//    // Minutes
//    if (d < 60) return std::to_string(d)+"m";
//    d /= 60;
//    // Hours
//    if (d < 24) return std::to_string(d)+"h";
//    d /= 24;
//    // Days
//    if (d < 7) return std::to_string(d)+"d";
//    d /= 7;
//    // Weeks
//    if (d < 52) return std::to_string(d)+"w";
//    d /= 52;
//    // Weeks
//    if (d < 52) return std::to_string(d)+"w";
//    d /= 52;
//    
//    
//    // Minutes
//    if (d < 60) return std::to_string(d)+"m";
}

} // namespace RelativeTime








//#pragma once
//#include <algorithm>
//#include "Window.h"
//
//namespace Time {
//
//inline std::string RelativeTimeDisplayString(uint64_t past) {
//    uint64_t current = time(nullptr);
//    if (past > current) return "future";
//    
//    static constexpr std::chrono::milliseconds _DoubleClickThresh(300);
//    
//    const uint64_t delta = current-past;
//    uint64_t d = current-past;
//    
//    // Seconds
//    if (d < 10) return "now";
//    if (d < 60) return std::to_string(d)+"s";
//    d /= 60;
//    // Minutes
//    if (d < 60) return std::to_string(d)+"m";
//    d /= 60;
//    // Hours
//    if (d < 24) return std::to_string(d)+"h";
//    d /= 24;
//    // Days
//    if (d < 7) return std::to_string(d)+"d";
//    d /= 7;
//    // Weeks
//    if (d < 52) return std::to_string(d)+"w";
//    d /= 52;
//    // Weeks
//    if (d < 52) return std::to_string(d)+"w";
//    d /= 52;
//    
//    
//    // Minutes
//    if (d < 60) return std::to_string(d)+"m";
//}
//
//} // namespace RelativeTime
