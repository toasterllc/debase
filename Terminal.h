#pragma once
#include <vector>
#include <string>
#include <optional>
#include <termios.h>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/Toastbox/ReadWrite.h"
#include "lib/Toastbox/StringSplit.h"
#include "lib/Toastbox/IntForStr.h"
#include "lib/Toastbox/Defer.h"

//// Terminal::_Parse() Fuzz testing (replaces ReadWrite.h)
//namespace Toastbox {
//
//template <typename T>
//T RandomInt(T max) {
//    if (!max) return 0;
//    return rand()%((int)max+1);
//}
//
//inline size_t Read(int fd, void* data, size_t len, std::chrono::steady_clock::time_point deadline=std::chrono::steady_clock::time_point()) {
//    assert(fd == 0);
//    assert(len == 1);
//    
//    static std::vector<uint8_t> randomData;
//    static bool replinish = true;
//    if (replinish) {
//        replinish = false;
//        
//        // Reproduce random data
//        size_t len = RandomInt(16);
//        for (size_t i=0; i<len; i++) {
//            int excess = 16;
//            randomData.push_back('0'+RandomInt(10+(2*excess))-excess);
//        }
//        
//        if (RandomInt(1)) {
//            randomData.insert(randomData.begin()+0, ';');
//            randomData.insert(randomData.begin()+1, 'r');
//            randomData.insert(randomData.begin()+2, 'g');
//            randomData.insert(randomData.begin()+3, 'b');
//            randomData.insert(randomData.begin()+4, ':');
//        }
//        
//        if (RandomInt(1)) {
//            for (int i=0; i<3; i++) {
//                randomData.insert(randomData.begin()+RandomInt(randomData.size()), '/');
//            }
//        }
//        
//        if (RandomInt(1)) {
//            randomData.insert(randomData.begin(), 0x1B);
//            randomData.push_back(0x07);
//        }
//    }
//    
//    if (randomData.empty()) {
//        replinish = true;
//        return 0;
//    }
//    
//    *(uint8_t*)data = randomData.front();
//    randomData.erase(randomData.begin());
//    return 1;
//}
//
//inline size_t Write(int fd, const void* data, size_t len, std::chrono::steady_clock::time_point deadline=std::chrono::steady_clock::time_point()) {
//    return len;
//}
//
//} // namespace Toastbox

namespace Terminal {

class Settings : public termios {
public:
    Settings(int fd) : _fd(fd) {
        _Get(_fd, *this);
    }
    
    ~Settings() {
        if (_prev) {
            restore();
        }
    }
    
    void set() {
        assert(!_prev);
        _prev.emplace();
        _Get(_fd, *_prev);
        _Set(_fd, *this);
    }
    
    void restore() {
        assert(_prev);
        _Set(_fd, *_prev);
        _prev = std::nullopt;
    }
    
private:
    using _Settings = struct termios;
    
    static void _Get(int fd, _Settings& out) {
        int ir = 0;
        do ir = tcgetattr(fd, &out);
        while (ir==-1 && errno==EINTR);
        if (ir) throw Toastbox::RuntimeError("tcgetattr failed: %s", strerror(errno));
    }
    
    static void _Set(int fd, const _Settings& out) {
        int ir = 0;
        do ir = tcsetattr(fd, TCSANOW, &out);
        while (ir==-1 && errno==EINTR);
        if (ir) throw Toastbox::RuntimeError("tcsetattr failed: %s", strerror(errno));
    }
    
//    static void _Swap() {
//        assert(!_rev);
//        do ir = tcsetattr(_fd, TCSANOW, &term);
//        while (ir==-1 && errno==EINTR);
//        if (ir) throw Toastbox::RuntimeError("tcsetattr failed: %s", strerror(errno));
//        // Restore terminal attributes on return
//        Defer(tcsetattr(_fd, TCSANOW, &termPrev));
//    }
    
    int _fd = -1;
    std::optional<struct termios> _prev;
};

enum class Background {
    Dark,
    Light,
};

struct _Color {
    float r = 0;
    float g = 0;
    float b = 0;
};

inline _Color _Parse(std::string_view str) {
    std::vector<std::string> parts = Toastbox::StringSplit(str, ";");
    if (parts.size() != 2) throw Toastbox::RuntimeError("invalid input string");
    
    parts = Toastbox::StringSplit(parts[1], ":");
    if (parts.size() != 2) throw Toastbox::RuntimeError("invalid input string");
    
    std::string colorspace = parts[0];
    std::transform(colorspace.begin(), colorspace.end(), colorspace.begin(), [](char c){ return std::tolower(c); });
    if (colorspace != "rgb") throw Toastbox::RuntimeError("unknown color space: %s", colorspace.c_str());
    
    std::string colorsStr = parts[1];
    if (colorsStr.empty()) throw Toastbox::RuntimeError("empty colors string");
    if (colorsStr[colorsStr.size()-1] != 0x07) throw Toastbox::RuntimeError("colors string doesn't end with BEL: %s", colorsStr.c_str());
    colorsStr.erase(colorsStr.end()-1);
    
    parts = Toastbox::StringSplit(colorsStr, "/");
    if (parts.size() != 3) throw Toastbox::RuntimeError("invalid color string count: %ju", (uintmax_t)parts.size());
    
    float colors[3] = {};
    for (size_t i=0; i<3; i++) {
        const std::string& colorStr = parts[i];
//        printf("str %zu: %s\n", i, colorStr.c_str());
        float val = Toastbox::IntForStr<uint16_t>(colorStr, 16);
        switch (colorStr.size()) {
        case 1:
        case 2:
        case 3:
        case 4: val /= ((1<<(4*colorStr.size()))-1); break;
        default: throw Toastbox::RuntimeError("invalid color string: %s", colorStr.c_str());
        }
        colors[i] = val;
    }
    
    return _Color{
        .r = colors[0],
        .g = colors[1],
        .b = colors[2],
    };
}

// Get(): returns the background color of the terminal, or throws if it couldn't be determined
inline Background BackgroundGet() {
    Settings settings(STDIN_FILENO);
    settings.c_lflag &= ~(ICANON|ECHO);
    settings.set();
    
//    struct termios termPrev;
//    int ir = 0;
//    do ir = tcgetattr(STDIN_FILENO, &termPrev);
//    while (ir==-1 && errno==EINTR);
//    if (ir) throw Toastbox::RuntimeError("tcgetattr failed: %s", strerror(errno));
//    
//    struct termios term = termPrev;
//    term.c_lflag &= ~(ICANON|ECHO);
//    do ir = tcsetattr(STDIN_FILENO, TCSANOW, &term);
//    while (ir==-1 && errno==EINTR);
//    if (ir) throw Toastbox::RuntimeError("tcsetattr failed: %s", strerror(errno));
//    // Restore terminal attributes on return
//    Defer(tcsetattr(STDIN_FILENO, TCSANOW, &termPrev));
    
    // Request terminal background color
    {
//        const uint8_t d[] = {0x1B,']','2','0',';','?',0x07}; // Invalid (for testing)
        const uint8_t d[] = {0x1B,']','1','1',';','?',0x07};
        Toastbox::Write(STDOUT_FILENO, d, sizeof(d));
    }
    
    {
        static constexpr std::chrono::milliseconds Timeout(500);
        auto deadline = std::chrono::steady_clock::now()+Timeout;
        
        uint8_t resp[32] = {};
        size_t off = 0;
        size_t sr = Toastbox::Read(STDIN_FILENO, resp, 1, deadline);
        if (sr != 1) throw Toastbox::RuntimeError("timeout receiving control sequence response");
        if (resp[0] != 0x1B) throw Toastbox::RuntimeError("first byte of control sequence response isn't 0x1B");
        off++;
        
        // Keep reading until we get a 0x07 (BELL) character
        for (;;) {
            if (off >= sizeof(resp)) throw Toastbox::RuntimeError("too many characters in control sequence response");
            size_t sr = Toastbox::Read(STDIN_FILENO, resp+off, 1, deadline);
            if (sr != 1) throw Toastbox::RuntimeError("timeout receiving control sequence response");
            off++;
            
            if (resp[off-1] == 0x07) break;
        }
        
        _Color c = _Parse(std::string_view((char*)resp, off));
        float avg = ((c.r+c.g+c.b)/3);
        if (avg < 0.5) return Background::Dark;
        return Background::Light;
    }
}

} // namespace TerminalBackground
