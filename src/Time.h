#pragma once
#include <chrono>

namespace Time {

struct Options {
    const char* pastSuffix   = "";
    const char* futureSuffix = "";
    bool abbreviate = false;
};

inline std::string _RelativeTimeString(bool ab, std::chrono::seconds sec) {
    using namespace std::chrono;
    using namespace std::string_literals;
    
    using Seconds   = duration<int64_t, std::ratio<1>>;
    using Minutes   = duration<int64_t, std::ratio<60>>;
    using Hours     = duration<int64_t, std::ratio<3600>>;
    using Days      = duration<int64_t, std::ratio<86400>>;
    using Weeks     = duration<int64_t, std::ratio<604800>>;
    using Months    = duration<int64_t, std::ratio<2629746>>;
    using Years     = duration<int64_t, std::ratio<31556952>>;
    
    const int64_t years   = duration_cast<Years>(sec).count();
    const int64_t months  = duration_cast<Months>(sec).count();
    const int64_t weeks   = duration_cast<Weeks>(sec).count();
    const int64_t days    = duration_cast<Days>(sec).count();
    const int64_t hours   = duration_cast<Hours>(sec).count();
    const int64_t minutes = duration_cast<Minutes>(sec).count();
    const int64_t seconds = duration_cast<Seconds>(sec).count();
    
    std::string unit = "";
         if (years >= 1)    return std::to_string(years)   + (ab ? "y" : (years<2   ? " year"   : " years"   ));
    else if (months >= 1)   return std::to_string(months)  + (ab ? "m" : (months<2  ? " month"  : " months"  ));
    else if (weeks >= 1)    return std::to_string(weeks)   + (ab ? "w" : (weeks<2   ? " week"   : " weeks"   ));
    else if (days >= 1)     return std::to_string(days)    + (ab ? "d" : (days<2    ? " day"    : " days"    ));
    else if (hours >= 1)    return std::to_string(hours)   + (ab ? "h" : (hours<2   ? " hour"   : " hours"   ));
    else if (minutes >= 1)  return std::to_string(minutes) + (ab ? "m" : (minutes<2 ? " minute" : " minutes" ));
    else if (seconds >= 10) return std::to_string(seconds) + (ab ? "s" : " seconds");
    return "";
}

inline std::string RelativeTimeString(const Options& opts, std::chrono::seconds sec) {
    using namespace std::chrono;
    
    const auto secAbs = std::chrono::abs(sec);
    const std::string base = _RelativeTimeString(opts.abbreviate, secAbs);
    if (base.empty()) return "now";
    
    std::string suffix = (sec<seconds(0) ? opts.pastSuffix : opts.futureSuffix);
    if (!suffix.empty()) suffix = " "+suffix;
    
    return base+suffix;
}

inline std::string RelativeTimeString(const Options& opts, std::chrono::system_clock::time_point t) {
    using namespace std::chrono;
    return RelativeTimeString(opts, duration_cast<seconds>(t-system_clock::now()));
}

inline std::string RelativeTimeString(const Options& opts, time_t t) {
    using namespace std::chrono;
    return RelativeTimeString(opts, system_clock::from_time_t(t));
}

} // namespace Time
