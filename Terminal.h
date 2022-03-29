#pragma once
#include <vector>
#include <string>
#include <termios.h>
#include "lib/Toastbox/RuntimeError.h"
#include "lib/Toastbox/ReadWrite.h"
#include "lib/Toastbox/StringSplit.h"
#include "lib/Toastbox/IntForStr.h"
#include "lib/Toastbox/Defer.h"

namespace Terminal {

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
    struct termios termPrev;
    int ir = 0;
    do ir = tcgetattr(0, &termPrev);
    while (ir==-1 && errno==EINTR);
    if (ir) throw Toastbox::RuntimeError("tcgetattr failed: %s", strerror(errno));
    
    struct termios term = termPrev;
    term.c_lflag &= ~(ICANON|ECHO);
    do ir = tcsetattr(0, TCSANOW, &term);
    while (ir==-1 && errno==EINTR);
    if (ir) throw Toastbox::RuntimeError("tcsetattr failed: %s", strerror(errno));
    // Restore terminal attributes on return
    Defer(tcsetattr(0, TCSANOW, &termPrev));
    
    // Request terminal background color
    {
//        const uint8_t d[] = {0x1B,']','2','0',';','?',0x07};
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
