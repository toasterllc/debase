#pragma once
#include <chrono>

namespace Time {

struct Options {
    const char* pastSuffix   = "";
    const char* futureSuffix = "";
    bool abbreviate = false;
};

inline std::string RelativeTimeDisplayString(const Options& opts, int64_t t) {
    using namespace std;
    using seconds   = chrono::duration<int64_t, std::ratio<1>>;
    using minutes   = chrono::duration<int64_t, std::ratio<60>>;
    using hours     = chrono::duration<int64_t, std::ratio<3600>>;
    using days      = chrono::duration<int64_t, std::ratio<86400>>;
    using weeks     = chrono::duration<int64_t, std::ratio<604800>>;
    using months    = chrono::duration<int64_t, std::ratio<2629746>>;
    using years     = chrono::duration<int64_t, std::ratio<31556952>>;
    
    const int64_t current = time(nullptr);
    const bool ab = opts.abbreviate;
    std::string suffix = (t<current ? opts.pastSuffix : opts.futureSuffix);
    if (!suffix.empty()) suffix = " "+suffix;
    seconds delta(std::abs(current-t));
    
    // Years
    int64_t d = chrono::duration_cast<years>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"y":" years")+suffix;
    // Months
    d = chrono::duration_cast<months>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"m":" months")+suffix;
    // Weeks
    d = chrono::duration_cast<weeks>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"w":" weeks")+suffix;
    // Days
    d = chrono::duration_cast<days>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"d":" days")+suffix;
    // Hours
    d = chrono::duration_cast<hours>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"h":" hours")+suffix;
    // Minutes
    d = chrono::duration_cast<minutes>(delta).count();
    if (d >= 1) return std::to_string(d)+(ab?"m":" minutes")+suffix;
    // Seconds
    d = chrono::duration_cast<seconds>(delta).count();
    if (d >= 10) return std::to_string(d)+(ab?"s":" seconds")+suffix;
    return "now";
}

} // namespace RelativeTime
